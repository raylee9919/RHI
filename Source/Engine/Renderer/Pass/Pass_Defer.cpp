// Copyright Seong Woo Lee. All Rights Reserved.

//
// Think of it as an implementation on the user/game side.
//
#include "Pass_Defer.h"

namespace Engine
{
    void Defer_Pass::draw(DX12_Command_List* cmd_list, void* param)
    {
        auto* data = (Draw_Data*)param;
        Push_Constants push = {
            // @Todo: Fragile af
            .position_id            = inputs[0]->get_srv_index(),
            .normal_id              = inputs[1]->get_srv_index(),
            .uv_id                  = inputs[2]->get_srv_index(),
            .material_id            = inputs[3]->get_srv_index(),
            .camera_id              = data->camera_id,
            .linear_sampler_id      = data->linear_sampler_id,
            .anisotropic_sampler_id = data->anisotropic_sampler_id
        };

        cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
        cmd_list->draw(3, 1);
    }
}
