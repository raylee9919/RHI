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

    FORCE_INLINE void alloc_material(Material m) { materials[m.name] = m; }
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

INTERNAL std::pair<DX12_Resource*, u32> alloc_resource_and_srv_and_return_id(DX12_Device* device, DX12_Descriptor_Heap* srv_heap, const nlohmann::json& json, const String& key, DXGI_FORMAT format, bool mip, u32 default_id)
{
    std::pair<DX12_Resource*, u32> result = { nullptr, default_id };
    u32 id = default_id;

    const String path = json[key].get<std::string>();
    const u16 mip_levels = mip ? 1 : 1; // @Temporary
    int x, y, forced_channels = 4;

    u8* image = stbi_load(path.c_str(), &x, &y, nullptr, forced_channels);

    if (image) {
        DX12_Resource_Desc desc = {
            .type = DX12_RESOURCE_TYPE_TEXTURE_2D,
            .texture = {
                .format      = format,
                .width       = (u32)x,
                .height      = (u32)y,
                .mip_levels  = mip_levels,
                .depth       = 1,
                .num_samples = 1,
            }
        };
        auto* tex = dx12_alloc_resource(device, desc);
        // @Todo: upload texture.
        auto srv = dx12_alloc_descriptor(srv_heap);
        dx12_create_srv(device, tex, &srv);
        id = srv.index;

        result.first  = tex;
        result.second = id;

        stbi_image_free(image);
    }

    return result;
}

INTERNAL Material material_from_json(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list, DX12_Fence* fence, DX12_Descriptor_Heap* srv_heap, const nlohmann::json& json)
{
    Material mat = {};

    mat.name = json["name"].get<std::string>();

    auto [albedo_tex, albedo_id]     = alloc_resource_and_srv_and_return_id(device, srv_heap, json,   "albedo", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [orm_tex, orm_id]           = alloc_resource_and_srv_and_return_id(device, srv_heap, json,      "orm", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [normal_tex, normal_id]     = alloc_resource_and_srv_and_return_id(device, srv_heap, json,   "normal", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);
    auto [emissive_tex, emissive_id] = alloc_resource_and_srv_and_return_id(device, srv_heap, json, "emissive", DXGI_FORMAT_R8G8B8A8_UNORM, true, 0xffffffff);

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

        auto compiled_vs = compiler->compile(true, source, length, "VS_Main", "vs_6_6");
        auto vs_reflection = compiler->reflect(compiled_vs.result);
        D3D12_INPUT_ELEMENT_DESC* vs_input_elements = new D3D12_INPUT_ELEMENT_DESC[vs_reflection.get_num_input_parameters()];
        vs_reflection.get_input_parameters(vs_input_elements);

        auto compiled_ps = compiler->compile(true, source, length, "PS_Main", "ps_6_6");


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
        const f32 fov          = to_radian(90.0f); // @Todo: degree and radian.
        const f32 aspect_ratio = 9.f / 16.f;
        const f32 near_z       = 0.1f;
        const f32 far_z        = 10000.0f;

        camera->position  = vec4(800.f, 300.f, 0.f, 1.f);
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
            cmd_list->clear_rtv(swap_chain->get_current_rtv(), 0.4f, 0.2f, 0.2f, 1.0f);

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
                        .vertex_buffer_id = slice.vertex_buffer_descriptor.index,
                        .material_id      = mat.id,
                        .camera_id        = camera_id
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
    dx12_destroy_device(device);

    deinit_shader_compiler(compiler);

    destroy_window(window);

    return 0;
}
