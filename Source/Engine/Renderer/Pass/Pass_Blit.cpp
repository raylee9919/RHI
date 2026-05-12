// Copyright Seong Woo Lee. All Rights Reserved.

//
// Think of it as an implementation on the user/game side.
//
#include "Pass_Blit.h"

namespace Engine
{
    void Blit_Pass::draw(DX12_Command_List* cmd_list, void* param)
    {
        auto* data = (Draw_Data*)param;

        Push_Constants push = {
            .color_id          = inputs[0]->get_srv_index(),
            .linear_sampler_id = data->linear_sampler_id
        };

        cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
        cmd_list->draw(3, 1);
    }
}
