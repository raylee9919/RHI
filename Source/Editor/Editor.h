// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Engine/Scene/Entity.h"

#include "ThirdParty/DearIMGUI/imgui.h"
#include "ThirdParty/DearIMGUI/imgui_impl_sdl3.h"
#include "ThirdParty/DearIMGUI/imgui_impl_dx12.h"
#include "ThirdParty/ImGUizmo/ImGuizmo.h"

namespace Engine
{
    struct Editor_State 
    {
        Entity::ID selected_entity_id;
        bool show_ui = false;
        ImGuizmo::OPERATION current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE;
    };
}
