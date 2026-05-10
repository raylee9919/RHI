// Copyright Seong Woo Lee. All Rights Reserved.

#include "ThirdParty/stb/Include/stb_image.h"

#include "Core/SE_Basics.h"
#include "Core/SE_String.h"
#include "Core/SE_Math.h"
#include "Window/Window.h"
#include "File/FileSystem.h"
#include "Asset/SE_Asset.h"
#include "RHI/DX12/DX12.h"
#include "Shader/DXIL/DXIL_Compiler.h"

using namespace Engine;
using namespace DXIL;

// Sync with shader!
struct GBuffer_Push_Constants {
    u32 vertex_buffer_id;
    u32 material_id;
    u32 camera_id;
    u32 anisotropic_sampler_id;
};

struct Defer_Push_Constants {
    u32 position_id;
    u32 normal_id;
    u32 uv_id;
    u32 material_id;
    u32 camera_id;
    u32 linear_sampler_id;
    u32 anisotropic_sampler_id;
};

struct Blit_Push_Constants {
    u32 color_id;
    u32 linear_sampler_id;
};

// Sync with shader!
struct Shader_Camera {
    m4x4 view;
    m4x4 proj;
    m4x4 view_proj;
    vec4 position;
};

struct Camera {
    vec3    position;
    f32     aspect_ratio;
    f32     fov;
    f32     near_z;
    f32     far_z;
    f32     yaw;
    f32     pitch;
    f32     speed;
    f32     last_mouse_x;
    f32     last_mouse_y;
    m4x4    view;
    m4x4    proj;
    m4x4    view_proj;

    void update(f32 dt, Input_System* input)
    {
        vec3 forward = vec3(0.f, 0.f, 1.f);
        vec4 f = y_rotation(yaw) * x_rotation(pitch) * vec4(forward, 0.f);
        forward = normalize(f.xyz);
        vec3 right = cross(forward, vec3(0.f, 1.f, 0.f));
        vec3 up = vec3(0.f, 1.f, 0.f);

        f32 move_speed = speed;
        if (input->key_is_down[KEY_LEFT_SHIFT]) {
            move_speed *= 3.0f;
        }

        f32 factor = dt * move_speed;

        if (input->key_is_down[KEY_E]) { position += ( up      * factor ); }
        if (input->key_is_down[KEY_Q]) { position -= ( up      * factor ); }
        if (input->key_is_down[KEY_W]) { position += ( forward * factor ); }
        if (input->key_is_down[KEY_S]) { position -= ( forward * factor ); }
        if (input->key_is_down[KEY_D]) { position += ( right   * factor ); }
        if (input->key_is_down[KEY_A]) { position -= ( right   * factor ); }

        if (input->mouse_is_down[MOUSE_LEFT]) {
            if (!input->mouse_was_down[MOUSE_LEFT]) {
                last_mouse_x = input->current_mouse_x;
                last_mouse_y = input->current_mouse_y;
            } else {
                f32 dx = input->current_mouse_x - last_mouse_x;
                f32 dy = input->current_mouse_y - last_mouse_y;

                f32 rot_speed = 0.25f;
                yaw   += (rot_speed * dx * dt);
                pitch -= (rot_speed * dy * dt);
                yaw   = fmod(yaw, PI * 2.f);
                pitch = Clamp(pitch, -PI * 0.45f, PI * 0.45f);

                last_mouse_x = input->current_mouse_x;
                last_mouse_y = input->current_mouse_y;
            }
        }

        view      = m4x4::look_to_lh(position, forward, vec3(0.f, 1.f, 0.f));
        proj      = m4x4::perspective_lh(fov, aspect_ratio, near_z, far_z);
        view_proj = proj * view;
    }
};

Shader_Camera get_shader_camera(Camera* camera) {
    Shader_Camera result = {
        .view      = camera->view,
        .proj      = camera->proj,
        .view_proj = camera->view_proj,
        .position  = vec4(camera->position, 1.0f),
    };
    return result;
}

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

struct Bitmap {
    u32 width;
    u32 height;
    u32 pitch;
    u8* data;
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
            u32 count  = (u32)slice->vertices.size();
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
            u32 count  = (u32)slice->indices.size();
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

    // @Todo: Better mipmap generation. SIMD or use compute shader.
    // Take sRGB textures into account.
    // Something better than box filter. JBlow's articles my be handy.
    //
    if (generate_mipmap) {
        u16 levels = (u16)floor(log2(max(x, y))) + 1;
        for (u16 i = 0; i < levels - 1; ++i) {
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
        u16 mip_levels = (u16)bitmaps.size();
        u32 tex_width  = bitmaps[0].width;
        u32 tex_height = bitmaps[0].height;

        DX12_Resource_Desc desc = {
            .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
            .texture = {
                .format      = format,
                .width       = tex_width,
                .height      = tex_height,
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
            cmd_list->transition_barrier(tex, 0, D3D12_RESOURCE_STATE_COPY_DEST);
            cmd_list->transition_barrier(upload_buffer, 0, D3D12_RESOURCE_STATE_COPY_SOURCE);

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

            cmd_list->transition_barrier(tex, 0, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(upload_buffer, 0, D3D12_RESOURCE_STATE_COMMON);
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
    // @Robustness
    auto [albedo_tex, albedo_id]     = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json,   "albedo", DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, true, 0xffffffff);
    auto [orm_tex, orm_id]           = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json,      "orm",      DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [normal_tex, normal_id]     = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json,   "normal",      DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [emissive_tex, emissive_id] = alloc_resource_and_srv_and_return_id(device, cmd_queue, cmd_list, fence, srv_heap, json, "emissive",      DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);

    mat.shader_material.albedo_id   = albedo_id;
    mat.shader_material.orm_id      = orm_id;
    mat.shader_material.normal_id   = normal_id;
    mat.shader_material.emissive_id = emissive_id;

    mat.resources.push_back(albedo_tex);
    mat.resources.push_back(orm_tex);
    mat.resources.push_back(normal_tex);
    mat.resources.push_back(emissive_tex);

    { // alloc material id.
        u32 size = (u32)sizeof(mat.shader_material);
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

void init_pipeline_state(DX12_Device* device, Shader_Compiler* compiler,
                         DX12_Pipeline_State* out_pso,
                         ID3D12RootSignature* root_signature,
                         DXGI_FORMAT* rtv_formats, 
                         bool has_depth, DXGI_FORMAT dsv_format,
                         D3D12_CULL_MODE cull_mode,
                         const String& path)
{
    // read file.
    u64 length = read_entire_file(path, nullptr);
    u8* source = new u8[length];
    read_entire_file(path, source);

    // entries/profiles
    const char* vs_entry   = "vs_main";
    const char* vs_profile = "vs_6_6";
    const char* ps_entry   = "ps_main";
    const char* ps_profile = "ps_6_6";

    // compile and reflect.
    auto compiled_vs   = compiler->compile(true, source, length, vs_entry, vs_profile);
    auto vs_reflection = compiler->reflect(compiled_vs.result);

    auto compiled_ps   = compiler->compile(true, source, length, ps_entry, ps_profile);
    auto ps_reflection = compiler->reflect(compiled_ps.result);

    u32 num_input_elements = vs_reflection.shader_desc.InputParameters;
    u32 num_render_targets = ps_reflection.shader_desc.OutputParameters;

    Array <D3D12_INPUT_ELEMENT_DESC> vs_input_elements(num_input_elements);
    vs_reflection.get_input_parameters(vs_input_elements.data());

    // pso creation
    DX12_Graphics_Pipeline_Desc pso_desc = {
        .root_signature     = root_signature,

        .num_input_elements = num_input_elements,
        .input_elements     = vs_input_elements.data(),

        .topology           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,

        .cull_mode          = cull_mode,

        .depth_enabled = has_depth,
        .num_render_targets = num_render_targets,
        .dsv_format  = dsv_format,

        .vs_bytecode = compiled_vs.bytecode,
        .vs_length   = compiled_vs.length, 

        .ps_bytecode = compiled_ps.bytecode,
        .ps_length   = compiled_ps.length, 
    };
    memcpy(&pso_desc.rtv_formats, rtv_formats, sizeof(pso_desc.rtv_formats[0]) * num_render_targets);

    *out_pso = dx12_create_graphics_pipeline_state(device, pso_desc);

    // Cleanup
    vs_reflection.release();
    compiled_vs.release();

    compiled_ps.release();
    ps_reflection.release();

    delete [] source;
}

struct Shader_Asset {
    String name;
    String shader_path;
    D3D12_CULL_MODE cull_mode;
    Array <DXGI_FORMAT> render_target_formats;
    bool has_depth;
    DXGI_FORMAT depth_format;
};

Shader_Asset shader_asset_from_file(const String& file_path) {
    std::ifstream f(file_path);
    nlohmann::json json = nlohmann::json::parse(f);


    // If key doesn't exist, it'll fail, which is neat.
    auto render_targets = json["render_targets"];
    String name         = json["name"];
    String shader_path  = json["shader"];
    String cull         = json["cull"];
    String depth_format = json["depth_format"];


    // Render target formats
    Array <DXGI_FORMAT> formats;
    for (auto& it : render_targets) {
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        String str = it.get<String>();
        if (str == "R32G32B32A32_FLOAT") {
            format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        } else if (str == "R8G8B8A8_UNORM") {
            format = DXGI_FORMAT_R8G8B8A8_UNORM;
        } else if (str == "R8G8B8A8_SNORM") {
            format = DXGI_FORMAT_R8G8B8A8_SNORM;
        } else if (str == "R32G32_FLOAT") {
            format = DXGI_FORMAT_R32G32_FLOAT;
        } else if (str == "R32_UINT") {
            format = DXGI_FORMAT_R32_UINT;
        } else {
            CORE_ASSERT(0);
        }
        formats.push_back(format);
    }

    // Cull mode
    D3D12_CULL_MODE cull_mode = D3D12_CULL_MODE_NONE;
    if (cull == "none") {
        cull_mode = D3D12_CULL_MODE_NONE;
    } else if (cull == "back") {
        cull_mode = D3D12_CULL_MODE_BACK;
    } else if (cull == "front") {
        cull_mode = D3D12_CULL_MODE_FRONT;
    } else {
        CORE_ASSERT(0);
    }

    // Depth
    bool has_depth = true;
    DXGI_FORMAT depth = DXGI_FORMAT_UNKNOWN;
    if (depth_format == "none") {
        has_depth = false;
    } else if (depth_format == "D24S8") {
        depth = DXGI_FORMAT_D24_UNORM_S8_UINT;
    } else {
        CORE_ASSERT(0);
    }

    Shader_Asset result = {};
    {
        result.name                  = name;
        result.shader_path           = shader_path;
        result.cull_mode             = cull_mode;
        result.render_target_formats = std::move(formats);
        result.has_depth             = has_depth;
        result.depth_format          = depth;
    }

    return result;
}

void init_pipeline_state_from_shader_asset(DX12_Device* device, Shader_Compiler* compiler, DX12_Pipeline_State* out_pso, ID3D12RootSignature* bindless_root_signature, const Shader_Asset& asset) {
    init_pipeline_state(device, compiler, out_pso, bindless_root_signature,
                        (DXGI_FORMAT*)asset.render_target_formats.data(), 
                        asset.has_depth, asset.depth_format, 
                        asset.cull_mode, asset.shader_path);
}

void deinit_pipeline_state(DX12_Pipeline_State* pso) {
    if (pso) {
        if (pso->pso) { pso->pso->Release(); }
    }
}

int main()
{
    Asset_State* asset_state = new Asset_State;
    file_sys::path project_dir  = "C:/dev/swl/RHI/Project";
    file_sys::path asset_dir    = project_dir / "Asset";
    file_sys::path material_dir = asset_dir / "Material";
    file_sys::path shader_dir   = asset_dir / "Shader";

    // Create a window.
    String title = "This is a window";
    int window_width  = 2560;
    int window_height = 1440;
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

    u32 tex_width  = 2560;
    u32 tex_height = 1440;
    u32 num_frames = 3;
    auto* swap_chain = dx12_create_swap_chain(device, cmd_queue, rtv_heap, (HWND)window->get_platform_window(), tex_width, tex_height, num_frames);

    auto* bindless_root_signature = dx12_create_bindless_root_signature(device);




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
    auto res = alloc_mesh_resource(device, cmd_queue, cmd_list, fence, res_heap, loaded_sponza.meshes[0]);
    resource_state->meshes[res.name] = res;

    // @Temporary: Hard coding a name for now.
    Entity* sponza = new Entity;
    sponza->mesh_name = "Sponza_mesh_0";


    // @Temporary
    //
    Camera camera; 
    {
        camera.position     = vec3(0.f, 300.f, 0.f);
        camera.aspect_ratio = 9.f / 16.f;
        camera.fov          = to_radian(90.0f);
        camera.near_z       = 0.1f;
        camera.far_z        = 10000.0f;
        camera.yaw          = PI * 0.5f;
        camera.pitch        = to_radian(0.f);
        camera.speed        = 128.f;
    };

    Shader_Camera shader_camera = get_shader_camera(&camera);
    auto* camera_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_BUFFER, .buffer = { .size = (u32)sizeof(shader_camera) } });
    auto camera_srv = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, camera_resource, &camera_srv, 1, (u32)sizeof(shader_camera));
    u32 camera_id = camera_srv.index;

    // @Temporary: Create linear sampler.
    //
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

    DXGI_FORMAT dsv_format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    auto* depth_texture_resource = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
                                                       .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                                       .do_clear = true,
                                                       .clear_value = { .Format = dsv_format, .DepthStencil = { .Depth = 1.0f, .Stencil = 0u }},
                                                       .texture = { .format = dsv_format,
                                                       .width = tex_width, .height = tex_height,
                                                       .mip_levels = 1, .depth = 1, .num_samples = 1 } });

    // Create DSV
    auto dsv = dx12_alloc_descriptor(dsv_heap);
    dx12_create_dsv(device, depth_texture_resource, &dsv, dsv_format);


    // Gbuffer
    //
    auto gbuffer_position_format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    auto* gbuffer_position = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D, 
                                                 .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                 .texture = { .format = gbuffer_position_format, .width = tex_width, .height = tex_height,
                                                 .mip_levels = 1, .depth = 1, .num_samples = 1 }, });
    auto gbuffer_position_rtv = dx12_alloc_descriptor(rtv_heap);
    dx12_create_rtv(device, gbuffer_position, &gbuffer_position_rtv, { .Format = gbuffer_position_format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
    auto gbuffer_position_srv = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, gbuffer_position, &gbuffer_position_srv);

    auto gbuffer_normal_format = DXGI_FORMAT_R8G8B8A8_SNORM;
    auto* gbuffer_normal = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D, 
                                                 .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                 .texture = { .format = gbuffer_normal_format, .width = tex_width, .height = tex_height,
                                                 .mip_levels = 1, .depth = 1, .num_samples = 1 }, });
    auto gbuffer_normal_rtv = dx12_alloc_descriptor(rtv_heap);
    dx12_create_rtv(device, gbuffer_normal, &gbuffer_normal_rtv, { .Format = gbuffer_normal_format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
    auto gbuffer_normal_srv = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, gbuffer_normal, &gbuffer_normal_srv);

    auto gbuffer_uv_format = DXGI_FORMAT_R32G32_FLOAT;
    auto* gbuffer_uv = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D, 
                                                 .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                 .texture = { .format = gbuffer_uv_format, .width = tex_width, .height = tex_height,
                                                 .mip_levels = 1, .depth = 1, .num_samples = 1 } });
    auto gbuffer_uv_rtv = dx12_alloc_descriptor(rtv_heap);
    dx12_create_rtv(device, gbuffer_uv, &gbuffer_uv_rtv, { .Format = gbuffer_uv_format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
    auto gbuffer_uv_srv = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, gbuffer_uv, &gbuffer_uv_srv);

    auto gbuffer_material_format = DXGI_FORMAT_R32_UINT;
    auto* gbuffer_material = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D, 
                                                 .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                                 .texture = { .format = gbuffer_material_format, .width = tex_width, .height = tex_height,
                                                 .mip_levels = 1, .depth = 1, .num_samples = 1 }, });
    auto gbuffer_material_rtv = dx12_alloc_descriptor(rtv_heap);
    dx12_create_rtv(device, gbuffer_material, &gbuffer_material_rtv, { .Format = gbuffer_material_format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
    auto gbuffer_material_srv = dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, gbuffer_material, &gbuffer_material_srv);

    // Defer
    //
    auto defer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
    auto* defer_texture = dx12_alloc_resource(device, { .type = DX12_RESOURCE_TYPE_TEXTURE_2D, 
                                              .resource_flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                              .texture = { .format = defer_format, .width = tex_width, .height = tex_height,
                                              .mip_levels = 1, .depth = 1, .num_samples = 1}});
    auto defer_rtv = dx12_alloc_descriptor(rtv_heap);
    dx12_create_rtv(device, defer_texture, &defer_rtv, { .Format = defer_format, .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D, .Texture2D = { .MipSlice = 0, .PlaneSlice = 0 } });
    auto defer_srv= dx12_alloc_descriptor(res_heap);
    dx12_create_srv(device, defer_texture, &defer_srv);

    // Blit
    //
    auto blit_format = swap_chain->get_current_resource()->desc.texture.format;
    String blit_shader_path = (asset_dir / "Shader/HLSL/Blit.shader_language").string();



    // Load shader meta data from disk.
    //
    Hash_Table <String, DX12_Pipeline_State*> shader_table;

    for (const auto& entry : file_sys::directory_iterator(shader_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            String file = entry.path().string();
            auto shader_asset = shader_asset_from_file(file);
            auto* shader = new DX12_Pipeline_State;
            init_pipeline_state_from_shader_asset(device, compiler, shader, bindless_root_signature, shader_asset);

            shader_table[shader_asset.name] = shader;
        }
    }








    // Main loop
    //
    while (window->is_running) {
        window->poll_events();

        // Update
        f32 time_elapsed = 0.017f;
        constexpr f32 dt = 1.f / 60.f;
        for (;time_elapsed >= dt; time_elapsed -= dt) {
            camera.update(dt, window->my_input_system);
        }

        // Upload camera resource
        shader_camera = get_shader_camera(&camera);
        dx12_upload_buffer(device, cmd_queue, cmd_list, fence, camera_resource, &shader_camera, (u32)sizeof(shader_camera));

        cmd_list->begin();
        {
            cmd_list->set_resource_and_sampler_heap(res_heap, sampler_heap);
            cmd_list->set_graphics_root_signature(bindless_root_signature);

            cmd_list->transition_barrier(depth_texture_resource, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            {
                cmd_list->clear_dsv(&dsv, 1.0f, 0, 0, tex_width, tex_height);
            }
            cmd_list->transition_barrier(depth_texture_resource, 0, D3D12_RESOURCE_STATE_COMMON);




            Array <GBuffer_Push_Constants> gbuffer_push_constants; 
            auto mesh = resource_state->meshes[sponza->mesh_name];
            for (auto& slice : mesh.slices) {
                auto mat = resource_state->materials[slice.material_name];
                GBuffer_Push_Constants push = {
                    .vertex_buffer_id       = slice.vertex_buffer_descriptor.index,
                    .material_id            = mat.id,
                    .camera_id              = camera_id,
                    .anisotropic_sampler_id = anisotropic_sampler_descriptor.index
                };
                gbuffer_push_constants.push_back(push);
            }
            u32 push_constants_size = sizeof(GBuffer_Push_Constants);


            auto* gbuffer_shader = shader_table["GBuffer"];
            cmd_list->transition_barrier(gbuffer_position, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->transition_barrier(gbuffer_normal, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->transition_barrier(gbuffer_uv, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->transition_barrier(gbuffer_material, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
            cmd_list->transition_barrier(depth_texture_resource, 0, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            {
                DX12_Descriptor rtvs[] = { gbuffer_position_rtv, gbuffer_normal_rtv, gbuffer_uv_rtv, gbuffer_material_rtv };
                cmd_list->set_render_target(count_of(rtvs), rtvs, &dsv);
                cmd_list->set_pipeline_state(gbuffer_shader);
                cmd_list->set_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                cmd_list->set_viewport(0, 0, tex_width, tex_height);
                cmd_list->set_scissor(0, 0, tex_width, tex_height);

                {
                    Entity* e = sponza;
                    auto mesh = resource_state->meshes[e->mesh_name];
                    for (u32 i = 0; i < mesh.slices.size(); ++i) {
                        auto& slice = mesh.slices[i];
                        cmd_list->set_graphics_root_constants(0u, push_constants_size >> 2, &gbuffer_push_constants[i]);
                        cmd_list->set_index_buffer(slice.index_buffer);
                        cmd_list->draw_indexed(slice.num_indices, 1);
                    }
                }
            }
            cmd_list->transition_barrier(gbuffer_position, 0, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(gbuffer_normal, 0, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(gbuffer_uv, 0, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(gbuffer_material, 0, D3D12_RESOURCE_STATE_COMMON);
            cmd_list->transition_barrier(depth_texture_resource, 0, D3D12_RESOURCE_STATE_COMMON);



            auto* defer_shader = shader_table["Defer"];
            cmd_list->transition_barrier(defer_texture, 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
            {
                DX12_Descriptor rtvs[] = { defer_rtv };
                cmd_list->set_render_target(count_of(rtvs), rtvs, &dsv);

                cmd_list->set_pipeline_state(defer_shader);
                cmd_list->set_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                cmd_list->set_viewport(0, 0, tex_width, tex_height);
                cmd_list->set_scissor(0, 0, tex_width, tex_height);

                {
                    Defer_Push_Constants push = {
                        .position_id            = gbuffer_position_srv.index,
                        .normal_id              = gbuffer_normal_srv.index,
                        .uv_id                  = gbuffer_uv_srv.index,
                        .material_id            = gbuffer_material_srv.index,
                        .camera_id              = camera_srv.index,
                        .linear_sampler_id      = linear_sampler_descriptor.index,
                        .anisotropic_sampler_id = anisotropic_sampler_descriptor.index
                    };

                    cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
                    cmd_list->draw(3, 1);
                }
            }
            cmd_list->transition_barrier(defer_texture, 0, D3D12_RESOURCE_STATE_COMMON);



            auto* blit_shader = shader_table["Blit"];
            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_RENDER_TARGET);
            {
                DX12_Descriptor rtvs[] = { swap_chain->get_current_rtv() };
                cmd_list->set_render_target(count_of(rtvs), rtvs, &dsv);

                cmd_list->set_pipeline_state(blit_shader);
                cmd_list->set_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
                cmd_list->set_viewport(0, 0, tex_width, tex_height);
                cmd_list->set_scissor(0, 0, tex_width, tex_height);

                {
                    Blit_Push_Constants push = {
                        .color_id = defer_srv.index,
                        .linear_sampler_id = linear_sampler_descriptor.index
                    };

                    cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
                    cmd_list->draw(3, 1);
                }
            }
            cmd_list->transition_barrier(swap_chain->get_current_resource(), 0, D3D12_RESOURCE_STATE_PRESENT);
        }
        cmd_list->end();

        dx12_execute_command_list(cmd_queue, cmd_list);

        dx12_signal_fence(cmd_queue, fence);
        dx12_wait_fence(fence);

        swap_chain->present();
    }

    return 0;
}
