// Copyright Seong Woo Lee. All Rights Reserved.

//
// Think of it as an implementation on the user/game side.
//
#include "Pass_GBuffer.h"

namespace Engine
{
    void GBuffer_Pass::draw(DX12_Command_List* cmd_list, void* param)
    {
        auto* data = (Draw_Data*)param;
        auto* resource_state = data->resource_state;
        auto* entity = data->entity;

        auto mesh = resource_state->meshes[entity->mesh_name];
        for (auto& slice : mesh.slices) {
            auto mat = resource_state->materials[slice.material_name];
            Push_Constants push = {
                .vertex_buffer_id       = slice.vertex_buffer_descriptor.index,
                .material_id            = mat.id,
                .camera_id              = data->camera_id,
                .anisotropic_sampler_id = data->anisotropic_sampler_id
            };

            auto& ib = slice.index_buffer;

            cmd_list->set_graphics_root_constants(0, sizeof(push) >> 2, &push);
            cmd_list->set_index_buffer(ib.resource);
            cmd_list->draw_indexed(ib.num_indices, 1);
        }
    }
}
