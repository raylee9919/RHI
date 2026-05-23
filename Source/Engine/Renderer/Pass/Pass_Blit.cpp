// Copyright Seong Woo Lee. All Rights Reserved.

#include "Pass_Blit.h"

namespace Engine
{
    void Blit_Pass::execute(Resource_State* resource_state, DX12_Command_List* cmd_list, Draw_Data data)
    {
        Push_Constants push = {
            .color_id  = resource_state->get_pass_resource(inputs[0]).get_srv_index(),
            .linear_id = data.linear_id
        };

        cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
        cmd_list->draw(3, 1);
    }
}
