// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Engine/Scene/Entity.h"
#include "Engine/RHI/DX12/DX12.h"

#include "Editor.h"

static Engine::DX12_Descriptor_Heap* g_imgui_srv_heap;

namespace Engine
{
    void ui_apply_style();

    void ui_begin();
    void ui_end();

    void ui_main_menu_bar();
    void ui_scene_hierarchy(Editor_State* editor, Scene* scene);
    void ui_entity_property(Editor_State* editor, Scene* scene);
    void ui_gizmo(Editor_State* editor, Scene* scene, m4x4 view, m4x4 proj, f32 viewport_x, f32 viewport_y, f32 viewport_width, f32 viewport_height);
}
