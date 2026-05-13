// Copyright Seong Woo Lee. All Rights Reserved.

//
// Think of it as an implementation on the user/game side.
//
#include "Pass_GBuffer.h"

namespace Engine
{
    void gbuffer_pass_dfs(Entity* entity, Scene* world, Resource_State* resource_state, DX12_Command_List* cmd_list, u32 transforms_id, u32 camera_id, u32 anisotropic_sampler_id)
    {
        auto mesh = resource_state->meshes[entity->mesh_name];
        for (auto& slice : mesh.slices) {
            auto mat = resource_state->materials[slice.material_name];
            GBuffer_Pass::Push_Constants push = {
                .vertex_buffer_id       = slice.vertex_buffer_descriptor.index,
                .transform_id           = entity->current_transform_index,
                .material_id            = mat.id,

                .transforms_id          = transforms_id,
                .camera_id              = camera_id,
                .anisotropic_sampler_id = anisotropic_sampler_id
            };

            auto& ib = slice.index_buffer;

            cmd_list->set_graphics_root_constants(0, sizeof(push) >> 2, &push);
            cmd_list->set_index_buffer(ib.resource);
            cmd_list->draw_indexed(ib.num_indices, 1);
        }

        for (auto id : entity->children) {
            auto* child = world->get_entity(id);
            if (child) {
                gbuffer_pass_dfs(child, world, resource_state, cmd_list, transforms_id, camera_id, anisotropic_sampler_id);
            }
        }
    }

    void GBuffer_Pass::draw(DX12_Command_List* cmd_list, void* param)
    {
        auto* data = (Draw_Data*)param;
        auto* resource_state = data->resource_state;
        auto* world = data->world;

        for (auto id : world->root->children) {
            auto* e = world->get_entity(id);
            if (e) {
                gbuffer_pass_dfs(e, world, resource_state, cmd_list, data->transforms_id, data->camera_id, data->anisotropic_sampler_id);
            }
        }
    }
}
