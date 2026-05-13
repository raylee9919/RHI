// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "ThirdParty/DearIMGUI/imgui.h"
#include "ThirdParty/DearIMGUI/imgui_impl_sdl3.h"
#include "ThirdParty/DearIMGUI/imgui_impl_dx12.h"
#include "ThirdParty/ImGUizmo/ImGuizmo.h"

#include "RHI/DX12/DX12.h"

static Engine::DX12_Descriptor_Heap* g_imgui_srv_heap;

namespace Editor
{
    void ui_apply_style();
}
