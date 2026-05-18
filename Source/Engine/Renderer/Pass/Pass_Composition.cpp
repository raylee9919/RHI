// Copyright Seong Woo Lee. All Rights Reserved.

#include "Pass_Composition.h"

namespace Engine
{
    void Composition_Pass::execute(Resource_State* resource_state, DX12_Command_List* cmd_list, Push_Constants push)
    {
        cmd_list->set_graphics_root_constants(0u, sizeof(push) >> 2, &push);
        cmd_list->draw(3, 1);
    }
}
