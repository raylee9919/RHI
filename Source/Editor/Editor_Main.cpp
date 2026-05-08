// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/SE_Basics.h"
#include "Core/SE_String.h"
#include "Core/SE_Math.h"
#include "Window/Window.h"
#include "File/FileSystem.h"
#include "Asset/SE_Asset.h"
#include "RHI/DX12/DX12.h"
#include "Shader/DXIL/DXIL_Compiler.h"

#include "ThirdParty/stb/Include/stb_image.h"

using namespace Engine;
using namespace DXIL;

// Sync with shader!
struct Push_Constants {
    u32 vertex_buffer_id;
    u32 material_id;
    u32 camera_id;
    u32 linear_sampler_id;
    u32 anisotropic_sampler_id;
};

// Sync with shader!
struct Camera {
    m4x4 view;
    m4x4 proj;
    m4x4 view_proj;
    vec4 position;
};

struct Entity {
    String mesh_name;
};

struct Mesh_Slice_Resource {
    DX12_Resource*  vertex_buffer;
    DX12_Descriptor vertex_buffer_descriptor;
    DX12_Resource*  index_buffer;

    String material_name;

    u32 num_vertices;
    u32 num_indices;
};

struct Mesh_Resource {
    String name;
    Array <Mesh_Slice_Resource> slices;
};

struct Resource_State {
    Hash_Table <String, Material> materials;
    Hash_Table <String, Mesh_Resource> meshes;

    void alloc_material(Material m) { materials[m.name] = m; }
    void clear()
    {
        Array <String> material_names;
        for (auto& it : materials) {
            dx12_dealloc_resource(it.second.resource);
            dx12_dealloc_descriptor(it.second.srv);
            for (auto* tex : it.second.resources) {
                dx12_dealloc_resource(tex);
            }
            material_names.push_back(it.first);
        }
        for (String& name : material_names) { materials.erase(name); }


        Array <String> mesh_names;
        for (auto& mesh : meshes) {
            for (auto& it : mesh.second.slices) {
                dx12_dealloc_resource(it.vertex_buffer);
                dx12_dealloc_descriptor(it.vertex_buffer_descriptor);
                dx12_dealloc_resource(it.index_buffer);
            }
            mesh_names.push_back(mesh.first);
        }
        for (String& name : mesh_names) { meshes.erase(name); }
    }
};

INTERNAL Mesh_Resource
alloc_mesh_resource(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list, DX12_Fence* fence, DX12_Descriptor_Heap* srv_heap, Mesh* mesh)
{
    Mesh_Resource result = {};
    result.name = mesh->name;

    for (u32 slice_index = 0; slice_index < mesh->meshes.size(); ++slice_index) {
        auto* slice = mesh->meshes[slice_index];
        Mesh_Slice_Resource slice_res = {};

        slice_res.material_name = slice->material_name;

        {
            u32 count  = slice->vertices.size();
            u32 stride = sizeof(slice->vertices[0]);
            u64 size   = count * stride;

            auto* vertex_buffer = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .heap_type = D3D12_HEAP_TYPE_DEFAULT, .buffer = { .size = size } });
            dx12_upload_buffer(device, cmd_queue, cmd_list, fence, vertex_buffer, slice->vertices.data(), size);

            DX12_Descriptor vertex_buffer_descriptor = dx12_alloc_descriptor(srv_heap);
            dx12_create_srv(device, vertex_buffer, &vertex_buffer_descriptor, count, stride);

            slice_res.vertex_buffer = vertex_buffer;
            slice_res.vertex_buffer_descriptor = vertex_buffer_descriptor;
            slice_res.num_vertices = count;
        }

        {
            u32 count  = slice->indices.size();
            u32 stride = sizeof(slice->indices[0]);
            u64 size   = count * stride;

            auto* index_buffer = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .heap_type = D3D12_HEAP_TYPE_DEFAULT, .buffer = { .size = size } });
            dx12_upload_buffer(device, cmd_queue, cmd_list, fence, index_buffer, slice->indices.data(), size);

            slice_res.index_buffer = index_buffer;
            slice_res.num_indices  = count;
        }

        result.slices.push_back(slice_res);
    }

    return result;
}

struct Bitmap {
    u32 width;
    u32 height;
    u32 pitch;
    u8* data;
};

INTERNAL Array <Bitmap> load_image(const String& path, bool generate_mipmap)
{
    Array <Bitmap> result = {};

    int x, y, forced_channels = 4;
    u8* image = stbi_load(path.c_str(), &x, &y, nullptr, forced_channels);
    assert(image);

    {
        Bitmap bitmap = {};
        {
            bitmap.width  = (u32)x;
            bitmap.height = (u32)y;
            bitmap.pitch  = bitmap.width * forced_channels;

            u64 sz = bitmap.pitch * bitmap.height;
            bitmap.data = new u8 [sz];
            memcpy(bitmap.data, image, sz);
        }

        result.push_back(bitmap);
    }

    if (generate_mipmap) {
        u16 levels = (u16)floor(log2(max(x, y))) + 1;
        for (u32 i = 0; i < levels - 1; ++i) {
            Bitmap* src = &result[i];
            Bitmap dst = {};

            dst.width  = max(1u, src->width  >> 1);
            dst.height = max(1u, src->height >> 1);
            dst.pitch  = dst.width * forced_channels;
            dst.data   = new u8 [dst.pitch * dst.height];

            for (u32 row = 0; row < dst.height; ++row) {
                for (u32 col = 0; col < dst.width; ++col) {
                    // Map dst texel to 4 src texels (2x2 box filter)
                    u32 sx = col * 2;
                    u32 sy = row * 2;

                    // Clamp in case src dimension was odd
                    u32 sx1 = min(sx + 1, src->width  - 1);
                    u32 sy1 = min(sy + 1, src->height - 1);

                    u8* s00 = src->data + sy  * src->pitch + sx  * forced_channels;
                    u8* s10 = src->data + sy  * src->pitch + sx1 * forced_channels;
                    u8* s01 = src->data + sy1 * src->pitch + sx  * forced_channels;
                    u8* s11 = src->data + sy1 * src->pitch + sx1 * forced_channels;

                    u8* d = dst.data + row * dst.pitch + col * forced_channels;

                    for (int c = 0; c < forced_channels; ++c) {
                        d[c] = (u8)((s00[c] + s10[c] + s01[c] + s11[c] + 2) >> 2);
                    }
                }
            }

            result.push_back(dst);
        }
    }

    stbi_image_free(image);

    return result;
}

INTERNAL std::pair<DX12_Resource*, u32> alloc_resource_and_srv_and_return_id(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list, DX12_Fence* fence, DX12_Descriptor_Heap* srv_heap, const nlohmann::json& json, const String& key, DXGI_FORMAT format, bool mip, u32 default_id)
{
    std::pair<DX12_Resource*, u32> result = { nullptr, default_id };
    DX12_Resource* tex = nullptr;
    u32 id = default_id;

    String path = json[key].get<std::string>();

    if (path != "") {
        auto bitmaps = load_image(path, mip);
        u16 mip_levels = bitmaps.size();
        u32 width      = bitmaps[0].width;
        u32 height     = bitmaps[0].height;

        DX12_Resource_Desc desc = {
            .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
            .texture = {
                .format      = format,
                .width       = width,
                .height      = height,
                .mip_levels  = mip_levels,
                .depth       = 1,
                .num_samples = 1,
            }
        };
        tex = dx12_alloc_resource(device, desc);

        D3D12_RESOURCE_DESC resource_desc = tex->native_resource->GetDesc();
        u32 num_subresources = resource_desc.DepthOrArraySize * resource_desc.MipLevels;
        Array <D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layouts(num_subresources);
        Array <u32> num_rows(num_subresources);
        Array <u64> row_size_in_bytes(num_subresources);
        u64 total_bytes;

        device->native_device->GetCopyableFootprints(&resource_desc, 0, num_subresources, 0, layouts.data(), num_rows.data(), row_size_in_bytes.data(), &total_bytes);

        DX12_Resource* upload_buffer = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .heap_type = D3D12_HEAP_TYPE_UPLOAD, .buffer = { .size = total_bytes } });
        const D3D12_RANGE read_range = { 0, 0 };
        void* ptr;
        upload_buffer->native_resource->Map(0, &read_range, &ptr);
        {
            for (u32 array_index = 0; array_index < resource_desc.DepthOrArraySize; ++array_index) {
                for (u32 mip_index = 0; mip_index < resource_desc.MipLevels; ++mip_index) {
                    u32 index = mip_index + array_index * resource_desc.MipLevels;
                    auto layout = layouts[index];
                    u32 subresource_height  = num_rows[index];
                    u32 subresource_pitch   = align_up(layout.Footprint.RowPitch, (u32)D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
                    u32 subresource_depth   = layout.Footprint.Depth;

                    u8* dst = (u8*)ptr + layout.Offset;

                    for (u32 slice_index = 0; slice_index < subresource_depth; ++slice_index) {
                        auto& bitmap = bitmaps[mip_index];
                        u8* src = bitmap.data;
                        for (u32 height = 0; height < subresource_height; ++height) {
                            memcpy(dst, src, min(subresource_pitch, bitmap.pitch));
                            dst += subresource_pitch;
                            src += bitmap.pitch;
                        }
                    }
                }
            }
        }
        upload_buffer->native_resource->Unmap(0, nullptr);

        // schedule upload
        cmd_list->begin();
        {
            cmd_list->transition_barrier(tex->native_resource, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
            cmd_list->transition_barrier(upload_buffer->native_resource, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

            for (u32 subresource_index = 0; subresource_index < num_subresources; ++subresource_index) {
                D3D12_TEXTURE_COPY_LOCATION dst = {
                    .pResource        = tex->native_resource,
                    .Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
                    .SubresourceIndex = subresource_index,
                };
                D3D12_TEXTURE_COPY_LOCATION src = {
                    .pResource        = upload_buffer->native_resource,
                    .Type             = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
                    .PlacedFootprint  = layouts[subresource_index]
                };
                cmd_list->native_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
            }

            cmd_list->transition_barrier(tex->native_resource, 0, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(upload_buffer->native_resource, 0, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
        }
        cmd_list->end();
        dx12_execute_command_list(cmd_queue, cmd_list);
        dx12_signal_fence(cmd_queue, fence);
        dx12_wait_fence(fence);
        dx12_dealloc_resource(upload_buffer);

        // allocate srv and get bindless id.
        auto srv = dx12_alloc_descriptor(srv_heap);
        dx12_create_srv(device, tex, &srv);
        id = srv.index;
    }

    result.first  = tex;
    result.second = id;

    return result;
}

INTERNAL Material material_from_json(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list, DX12_Fence* fence, DX12_Descriptor_Heap* srv_heap, const nlohmann::json& json)
{
    Material mat = {};

    mat.name = json["name"].get<std::string>();

    // @Todo: default material id
    auto [albedo_tex, albedo_id]     = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json,   "albedo", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [orm_tex, orm_id]           = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json,      "orm", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [normal_tex, normal_id]     = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json,   "normal", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [emissive_tex, emissive_id] = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json, "emissive", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);

    mat.shader_material.albedo_id   = albedo_id;
    mat.shader_material.orm_id      = orm_id;
    mat.shader_material.normal_id   = normal_id;
    mat.shader_material.emissive_id = emissive_id;

    mat.resources.push_back(albedo_tex);
    mat.resources.push_back(orm_tex);
    mat.resources.push_back(normal_tex);
    mat.resources.push_back(emissive_tex);

    { // alloc material id.
        u64 size = sizeof(mat.shader_material);
        auto* mat_res = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .buffer = { .size = size }});
        dx12_upload_buffer(device, cmd_queue, cmd_list, fence, mat_res, &mat.shader_material, size);
        auto srv = dx12_alloc_descriptor(srv_heap);
        dx12_create_srv(device, mat_res, &srv, 1, size);

        mat.id       = srv.index;
        mat.resource = mat_res;
        mat.srv      = srv;
    }

    return mat;
}

int main()
{
    Asset_State* asset_state = new Asset_State;
    file_sys::path project_dir  = "C:/dev/swl/RHI/Project";
    file_sys::path asset_dir    = project_dir / "Asset";
    file_sys::path material_dir = asset_dir / "Material";

    // Create a window.
    String title = "This is a window";
    int window_width  = 1920;
    int window_height = 1080;
    Window* window = create_window(title, window_width, window_height);

    // Resource state
    Resource_State* resource_state = new Resource_State;

    Shader_Compiler* compiler = new Shader_Compiler;
    init_shader_compiler(compiler);

    auto* device    = dx12_create_device();
    auto* cmd_queue = dx12_create_command_queue(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* cmd_list  = dx12_create_command_list(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto* fence     = dx12_create_fence(device);

    auto* rtv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,          32);
    auto* dsv_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV,          32);
    auto* res_heap     = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 512);
    auto* sampler_heap = dx12_create_descriptor_heap(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,      32);

    u32 tex_width  = 1920;
    u32 tex_height = 1080;
    u32 num_frames = 3;
    auto* swap_chain = dx12_create_swap_chain(device, cmd_queue, rtv_heap, (HWND)window->get_platform_window(), tex_width, tex_height, num_frames);

    DXGI_FORMAT rtv_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    DXGI_FORMAT dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    auto* color_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                       .texture = { .format = rtv_format,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    auto* depth_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                                       .do_clear = true,
                                                       .clear_value = { .Format = dsv_format, .DepthStencil = { .Depth = 1.0f, .Stencil = 0u }},
                                                       .texture = { .format = dsv_format,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    auto* bindless_root_signature = dx12_create_bindless_root_signature(device);

    auto* pso = new DX12_Pipeline_State;
    {
        // Reflection
        String path = (asset_dir / "Shader/HLSL/Triangle.shader").string();
        u64 length = read_entire_file(path, nullptr);
        u8* source = new u8[length];
        read_entire_file(path, source);

        auto compiled_vs = compiler->compile(true, source, length, "vs_main", "vs_6_6");
        auto vs_reflection = compiler->reflect(compiled_vs.result);
        D3D12_INPUT_ELEMENT_DESC* vs_input_elements = new D3D12_INPUT_ELEMENT_DESC[vs_reflection.get_num_input_parameters()];
        vs_reflection.get_input_parameters(vs_input_elements);

        auto compiled_ps = compiler->compile(true, source, length, "ps_main", "ps_6_6");


        // PSO creation
        DX12_Graphics_Pipeline_Desc pso_desc = {
            .root_signature     = bindless_root_signature,

            .num_input_elements = vs_reflection.get_num_input_parameters(),
            .input_elements     = vs_input_elements,

            .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,

            .cull_mode          = D3D12_CULL_MODE_BACK,

            .depth_enabled = true,
            .num_render_targets = 1,
            .rtv_formats = {
                rtv_format
            },
            .dsv_format  = dsv_format,

            .vs_bytecode = compiled_vs.bytecode,
            .vs_length   = compiled_vs.length, 

            .ps_bytecode = compiled_ps.bytecode,
            .ps_length   = compiled_ps.length, 
        };
        *pso = dx12_create_graphics_pipeline_state(device, pso_desc);


        // Cleanup
        delete [] vs_input_elements;
        vs_reflection.release();
        compiled_vs.release();

        compiled_ps.release();

        delete [] source;
    }

    // @Temporary
    Camera* camera = new Camera;
    {
        const f32 fov          = to_radian(90.0f);
        const f32 aspect_ratio = 9.f / 16.f;
        const f32 near_z       = 0.1f;
        const f32 far_z        = 10000.0f;

        camera->position  = vec4(800.f, 200.f, 0.f, 1.f);
        camera->view      = m4x4::look_at_lh(camera->position.xyz, vec3(0.f), vec3(0.f, 1.f, 0.f));
        camera->proj      = m4x4::perspective_lh(fov, aspect_ratio, near_z, far_z);
        camera->view_proj = camera->proj * camera->view;
    }
    u64 size = sizeof(*camera);
    auto* camera_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .buffer = { .size = size } });
    dx12_upload_buffer(device, cmd_queue, cmd_list, fence, camera_resource, camera, size);
    auto camera_srv = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, camera_resource, &camera_srv, 1, size);
    u32 camera_id = camera_srv.index;


    // @Temporary: Create linear sampler.
    auto linear_sampler_descriptor = dx12_alloc_descriptor(sampler_heap);
    {
        D3D12_SAMPLER_DESC desc = {
            .Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU       = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 
            .AddressV       = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 
            .AddressW       = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 
            .MipLODBias     = 0.f,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .MinLOD         = 0.f,
            .MaxLOD         = 1e10f
        };
        device->native_device->CreateSampler(&desc, linear_sampler_descriptor.cpu_handle);
    }

    // @Temporary: Create linear sampler.
    auto anisotropic_sampler_descriptor = dx12_alloc_descriptor(sampler_heap);
    {
        u32 x = 16;
        D3D12_SAMPLER_DESC desc = {
            .Filter         = D3D12_FILTER_ANISOTROPIC,
            .AddressU       = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 
            .AddressV       = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 
            .AddressW       = D3D12_TEXTURE_ADDRESS_MODE_MIRROR, 
            .MipLODBias     = 0.f,
            .MaxAnisotropy  = x,
            .ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
            .MinLOD         = 0.f,
            .MaxLOD         = 1e10f
        };
        device->native_device->CreateSampler(&desc, anisotropic_sampler_descriptor.cpu_handle);
    }


    // Import Spozna GLTF "manually".
    String sponza_path = (asset_dir / "Model/Sponza/Sponza.gltf").string();
    auto loaded_sponza = load_gltf(asset_state, sponza_path);

    // json will be a representation of material asset for now.
    // Either you import from an authored asset (gltf, fbx...) or load 
    // serialized json material from disk, it'll be there.
    auto& json_array = loaded_sponza.materials;

    for (const auto& json : json_array) {
        Material mat = material_from_json(device, cmd_queue, cmd_list, fence, res_heap, json);
        resource_state->alloc_material(mat);
    }

    // Alloc mesh resources.
    {
        auto res = alloc_mesh_resource(device, cmd_queue, cmd_list, fence, res_heap, loaded_sponza.meshes[0]);
        resource_state->meshes[res.name] = res;
    }

    // @Temporary: Hard coding a name for now.
    Entity* sponza = new Entity;
    sponza->mesh_name = "Sponza_mesh_0";


    // Create DSV
    auto dsv = dx12_alloc_descriptor(dsv_heap);
    dx12_create_dsv(device, depth_texture_resource, &dsv, dsv_format);


    // Main loop
    //
    while (window->is_running) {
        while (window->poll_events()) {}

        cmd_list->begin();
        {
            cmd_list->set_viewport(0, 0, tex_width, tex_height);
            cmd_list->set_scissor(0, 0, tex_width, tex_height);

            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->clear_rtv(swap_chain->get_current_rtv(), 1.0f, 0.0f, 1.0f, 0.0f);

            cmd_list->transition_barrier(depth_texture_resource->native_resource, 0, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            cmd_list->clear_dsv(&dsv, 1.0f, 0, 0, tex_width, tex_height);

            cmd_list->set_resource_and_sampler_heap(res_heap, sampler_heap);

            cmd_list->set_graphics_root_signature(bindless_root_signature);
            cmd_list->set_pipeline_state(pso);
            cmd_list->set_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            DX12_Descriptor *rtvs[] = { swap_chain->get_current_rtv() };
            cmd_list->set_render_target(1, rtvs, &dsv);

            {
                Entity* e = sponza;
                auto mesh = resource_state->meshes[e->mesh_name];
                for (auto& slice : mesh.slices) {
                    auto mat = resource_state->materials[slice.material_name];
                    Push_Constants pc = {
                        .vertex_buffer_id       = slice.vertex_buffer_descriptor.index,
                        .material_id            = mat.id,
                        .camera_id              = camera_id,
                        .linear_sampler_id      = linear_sampler_descriptor.index,
                        .anisotropic_sampler_id = anisotropic_sampler_descriptor.index
                    };

                    cmd_list->set_graphics_root_constants(0u, sizeof(Push_Constants) >> 2, &pc);
                    cmd_list->set_index_buffer(slice.index_buffer);
                    cmd_list->draw_indexed(slice.num_indices, 1);
                }
            }

            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            cmd_list->transition_barrier(depth_texture_resource->native_resource, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON);
        }
        cmd_list->end();

        dx12_execute_command_list(cmd_queue, cmd_list);

        dx12_signal_fence(cmd_queue, fence);
        dx12_wait_fence(fence);

        swap_chain->present();
    }

    // Cleanups
    //
    pso->release();

    dx12_signal_fence(cmd_queue, fence);
    dx12_wait_fence(fence);

    bindless_root_signature->Release();

    dx12_dealloc_resource(color_texture_resource);
    dx12_dealloc_resource(depth_texture_resource);

    dx12_destroy_swap_chain(swap_chain);
    dx12_destroy_descriptor_heap(rtv_heap);
    dx12_destroy_descriptor_heap(dsv_heap);
    dx12_destroy_descriptor_heap(res_heap);
    dx12_destroy_descriptor_heap(sampler_heap);
    dx12_destroy_fence(fence);
    dx12_destroy_command_list(cmd_list);
    dx12_destroy_command_queue(cmd_queue);
    dx12_dealloc_resource(camera_resource);
    resource_state->clear();

    dx12_destroy_device(device);

    deinit_shader_compiler(compiler);

    destroy_window(window);


    return 0;
}
