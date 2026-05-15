// Copyright Seong Woo Lee. All Rights Reserved.

//
// Think of it as an implementation on the user/game side.
//
#include "Pass_Defer.h"

namespace Engine
{
    void Defer_Pass::execute(Resource_State* resource_state, DX12_Command_List* cmd_list, Draw_Data data)
    {
        Push_Constants push = {
            // @Robustness
            .position_id            = resource_state->get_pass_resource(inputs[0]).get_srv_index(),
            .normal_id              = resource_state->get_pass_resource(inputs[1]).get_srv_index(),
            .uv_id                  = resource_state->get_pass_resource(inputs[2]).get_srv_index(),
            .material_id            = resource_state->get_pass_resource(inputs[3]).get_srv_index(),

            .camera_id              = data.camera_id,
            .linear_sampler_id      = data.linear_id,
            .anisotropic_sampler_id = data.anisotropic_id
        };

        cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
        cmd_list->draw(3, 1);
    }
}
