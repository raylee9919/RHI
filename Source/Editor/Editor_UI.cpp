// Copyright Seong Woo Lee. All Rights Reserved.

#include "Editor_UI.h"

#include <functional>

namespace Engine
{
    void ui_begin()
    {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGuizmo::AllowAxisFlip(false);
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::BeginFrame();
    }

    void ui_end()
    {
        ImGui::Render(); // CPU-side
    }

    void ui_main_menu_bar()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New")) {
                }
                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                }
                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                }
                if (ImGui::MenuItem("Save As..")) {
                }

                ImGui::Separator();
                if (ImGui::MenuItem("Quit", "Alt+F4")) {
                }
                ImGui::EndMenu();
            }
            ImGui::EndMainMenuBar();
        }
    }

    void ui_scene_hierarchy(Editor_State* editor, Scene* scene)
    {
        ImGui::Begin("Scene Hierarchy");
        {
            ImGuiTreeNodeFlags base_flags = (ImGuiTreeNodeFlags_DrawLinesFull
                                             | ImGuiTreeNodeFlags_DefaultOpen
                                             | ImGuiTreeNodeFlags_OpenOnArrow);

            std::function<void(Entity*)> draw_entity = [&](Entity* entity) {
                ImGuiTreeNodeFlags flags = base_flags;
                if (entity->is_leaf()) {
                    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }

                bool opened = ImGui::TreeNodeEx(entity->name.c_str(), flags);

                if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                    editor->selected_entity_id = entity->id;
                }

                if (opened && !entity->is_leaf()) {
                    for (auto child_id : entity->children) {
                        auto* child = scene->get_entity(child_id);
                        draw_entity(child);
                    }
                    ImGui::TreePop();
                }
            };

            for (auto id : scene->root->children) {
                auto* root_entity = scene->get_entity(id);
                draw_entity(root_entity);
            }
        }
        ImGui::End();
    }

    void ui_entity_property(Editor_State* editor, Scene* scene)
    {
        Entity* entity = scene->get_entity(editor->selected_entity_id);
        if (!entity) return;

        ImGui::Begin("Property Panel");
        {
            ImGui::Text("%s", entity->name.c_str());

            { // Transform
                ImGui::SeparatorText("Transform");

                // Position
                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", &entity->position.x, 0.01f, -FLT_MAX, FLT_MAX, "%.2f m");

                // Rotation (Quaternion -> Euler)
                //vec3 euler_deg = quat_to_euler_deg(entity->orientation);

                //if (ImGui::DragFloat3("Rotation", &euler_deg.x, 1.0f)) {
                //    transform.orientation = euler_deg_to_quat(euler_deg);
                //}

                // Scale
                ImGui::Text("Scale");
                ImGui::DragFloat3("##Scale", &entity->scaling.x, 0.01f, 0.00001f, 1000.0f);
            }
        }
        ImGui::End();
    }

    void ui_gizmo(Editor_State* editor, Scene* scene, m4x4 view, m4x4 proj, f32 viewport_x, f32 viewport_y, f32 viewport_width, f32 viewport_height)
    {
        Entity* entity = scene->get_entity(editor->selected_entity_id);
        if (entity) {
            m4x4 matrix;
            vec3 euler = to_euler(entity->orientation);
            vec3 orientation = vec3(euler.x, euler.y, euler.z); // Swizzle?
            orientation.x = to_radian(orientation.x);
            orientation.y = to_radian(orientation.y);
            orientation.z = to_radian(orientation.z);

            view = transpose(view);
            proj = transpose(proj);

            if (ImGui::IsKeyPressed(ImGuiKey_I)) { editor->current_gizmo_operation = ImGuizmo::OPERATION::TRANSLATE; }
            if (ImGui::IsKeyPressed(ImGuiKey_O)) { editor->current_gizmo_operation = ImGuizmo::OPERATION::ROTATE;    }
            if (ImGui::IsKeyPressed(ImGuiKey_P)) { editor->current_gizmo_operation = ImGuizmo::OPERATION::SCALE;     }

            //auto mode = editor->current_gizmo_operation == ImGuizmo::OPERATION::SCALE ? ImGuizmo::MODE::LOCAL : ImGuizmo::MODE::WORLD;
            auto mode = ImGuizmo::MODE::WORLD;
            ImGuizmo::RecomposeMatrixFromComponents(&entity->position.x, &orientation.x, &entity->scaling.x, &matrix._11);
            ImGuizmo::SetRect(viewport_x, viewport_y, viewport_width, viewport_height);
            ImGuizmo::Manipulate(&view._11, &proj._11, editor->current_gizmo_operation, mode, &matrix._11);
            ImGuizmo::DecomposeMatrixToComponents(&matrix._11, &entity->position.x, &orientation.x, &entity->scaling.x);

            orientation.x = to_degree(orientation.x);
            orientation.y = to_degree(orientation.y);
            orientation.z = to_degree(orientation.z);
            entity->orientation = to_quaternion(orientation);
        }
    }

    void ui_apply_style()
    {
        ImGuiStyle& Style = ImGui::GetStyle();

        const float Hue = 0.0f; // [0,1] range.
        const float Saturation = 1.0f; // [0,6] range.
        const float SaturationAccent = 1.0f; // [0,6] range.
        const float Transparency = 0.95f;
        const float BorderSize = 0.0f;

        Style.FrameBorderSize = BorderSize;
        Style.ImageBorderSize = BorderSize;
        Style.TabBorderSize = BorderSize;
        Style.TabBarBorderSize = 3.0f;
        Style.WindowRounding = 4.0f;
        Style.ChildRounding = 4.0f;
        Style.FrameRounding = 4.0f;
        Style.GrabRounding = 4.0f;
        Style.TabRounding = 4.0f;

        ImVec4 TextColor{1.000f, 1.000f, 1.000f, 1.000f};
        ImVec4 TextDimmedColor{0.357f, 0.482f, 0.549f, 1.000f};
        ImVec4 BackroundColor{0.110f, 0.149f, 0.169f, 1.000f};
        ImVec4 BackroundChildColor{0.090f, 0.122f, 0.141f, 1.000f};
        ImVec4 BackroundDimmedColor{0.000f, 0.000f, 0.000f, 0.600f};
        ImVec4 TitleColor{0.078f, 0.102f, 0.122f, 1.000f};
        ImVec4 HeaderColor{0.184f, 0.247f, 0.286f, 1.000f};
        ImVec4 Accent1Color{0.000f, 0.490f, 1.000f, 1.000f};
        ImVec4 Accent2Color{0.000f, 0.412f, 0.824f, 1.000f};
        ImVec4 Accent1AlternativeColor{0.302f, 0.408f, 0.475f, 1.000f};
        ImVec4 Accent2AlternativeColor{0.251f, 0.337f, 0.392f, 1.000f};
        ImVec4 TransparentColor{0.0f, 0.0f, 0.0f, 0.0f};

        auto AdjustHueAndSaturation = [](const ImVec4& Color, const float HueShift, const float SaturationScale) -> ImVec4
        {
            ImVec4 ResultColor = Color;
            ImGui::ColorConvertRGBtoHSV(ResultColor.x, ResultColor.y, ResultColor.z, ResultColor.x, ResultColor.y, ResultColor.z);

            ResultColor.x += HueShift;
            ResultColor.y *= SaturationScale;

            ImGui::ColorConvertHSVtoRGB(ResultColor.x, ResultColor.y, ResultColor.z, ResultColor.x, ResultColor.y, ResultColor.z);
            return ResultColor;
        };

        TextColor               = AdjustHueAndSaturation(TextColor, Hue, Saturation);
        TextDimmedColor         = AdjustHueAndSaturation(TextDimmedColor, Hue, Saturation);
        BackroundColor          = AdjustHueAndSaturation(BackroundColor, Hue, Saturation);
        BackroundChildColor     = AdjustHueAndSaturation(BackroundChildColor, Hue, Saturation);
        BackroundDimmedColor    = AdjustHueAndSaturation(BackroundDimmedColor, Hue, Saturation);
        TitleColor              = AdjustHueAndSaturation(TitleColor, Hue, Saturation);
        HeaderColor             = AdjustHueAndSaturation(HeaderColor, Hue, Saturation);
        Accent1Color            = AdjustHueAndSaturation(Accent1Color, Hue, SaturationAccent);
        Accent2Color            = AdjustHueAndSaturation(Accent2Color, Hue, SaturationAccent);
        Accent1AlternativeColor = AdjustHueAndSaturation(Accent1AlternativeColor, Hue, SaturationAccent);
        Accent2AlternativeColor = AdjustHueAndSaturation(Accent2AlternativeColor, Hue, SaturationAccent);

        ImVec4 BackroundTransparentColor = BackroundColor;
        BackroundTransparentColor.w *= Transparency;

        ImVec4 BackroundChildTransparentColor = BackroundChildColor;
        BackroundChildTransparentColor.w *= Transparency;

        ImVec4 TitleTransparentColor = TitleColor;
        TitleTransparentColor.w *= Transparency;

        ImVec4 HeaderTransparentColor = HeaderColor;
        HeaderTransparentColor.w *= Transparency;

        Style.Colors[ImGuiCol_Text] = TextColor;
        Style.Colors[ImGuiCol_TextDisabled] = TextDimmedColor;
        Style.Colors[ImGuiCol_WindowBg] = BackroundTransparentColor;
        Style.Colors[ImGuiCol_ChildBg] = BackroundChildTransparentColor;
        Style.Colors[ImGuiCol_PopupBg] = BackroundChildTransparentColor;
        Style.Colors[ImGuiCol_Border] = TitleColor;
        Style.Colors[ImGuiCol_BorderShadow] = TransparentColor;
        Style.Colors[ImGuiCol_FrameBg] = HeaderTransparentColor;
        Style.Colors[ImGuiCol_FrameBgHovered] = Accent1AlternativeColor;
        Style.Colors[ImGuiCol_FrameBgActive] = Accent2AlternativeColor;
        Style.Colors[ImGuiCol_TitleBg] = HeaderTransparentColor;
        Style.Colors[ImGuiCol_TitleBgActive] = TitleTransparentColor;
        Style.Colors[ImGuiCol_TitleBgCollapsed] = HeaderTransparentColor;
        Style.Colors[ImGuiCol_MenuBarBg] = BackroundChildTransparentColor;
        Style.Colors[ImGuiCol_ScrollbarBg] = TitleTransparentColor;
        Style.Colors[ImGuiCol_ScrollbarGrab] = HeaderColor;
        Style.Colors[ImGuiCol_ScrollbarGrabHovered] = Accent1AlternativeColor;
        Style.Colors[ImGuiCol_ScrollbarGrabActive] = Accent2AlternativeColor;
        Style.Colors[ImGuiCol_CheckMark] = Accent1Color;
        Style.Colors[ImGuiCol_SliderGrab] = Accent1Color;
        Style.Colors[ImGuiCol_SliderGrabActive] = Accent2Color;
        Style.Colors[ImGuiCol_Button] = HeaderTransparentColor;
        Style.Colors[ImGuiCol_ButtonHovered] = Accent1Color;
        Style.Colors[ImGuiCol_ButtonActive] = Accent2Color;
        Style.Colors[ImGuiCol_Header] = HeaderTransparentColor;
        Style.Colors[ImGuiCol_HeaderHovered] = Accent1Color;
        Style.Colors[ImGuiCol_HeaderActive] = Accent2Color;
        Style.Colors[ImGuiCol_Separator] = HeaderColor;
        Style.Colors[ImGuiCol_SeparatorHovered] = Accent1Color;
        Style.Colors[ImGuiCol_SeparatorActive] = Accent2Color;
        Style.Colors[ImGuiCol_ResizeGrip] = HeaderColor;
        Style.Colors[ImGuiCol_ResizeGripHovered] = Accent1Color;
        Style.Colors[ImGuiCol_ResizeGripActive] = Accent2Color;
        Style.Colors[ImGuiCol_InputTextCursor] = TextColor;
        Style.Colors[ImGuiCol_TabHovered] = Accent1Color;
        Style.Colors[ImGuiCol_Tab] = TransparentColor;
        Style.Colors[ImGuiCol_TabSelected] = HeaderTransparentColor;
        Style.Colors[ImGuiCol_TabSelectedOverline] = TransparentColor;
        Style.Colors[ImGuiCol_TabDimmed] = TransparentColor;
        Style.Colors[ImGuiCol_TabDimmedSelected] = BackroundChildTransparentColor;
        Style.Colors[ImGuiCol_TabDimmedSelectedOverline] = TransparentColor;
        Style.Colors[ImGuiCol_PlotLines] = TextDimmedColor;
        Style.Colors[ImGuiCol_PlotLinesHovered] = Accent1Color;
        Style.Colors[ImGuiCol_PlotHistogram] = TextDimmedColor;
        Style.Colors[ImGuiCol_PlotHistogramHovered] = Accent1Color;
        Style.Colors[ImGuiCol_TableHeaderBg] = BackroundChildTransparentColor;
        Style.Colors[ImGuiCol_TableBorderStrong] = TitleColor;
        Style.Colors[ImGuiCol_TableBorderLight] = HeaderColor;
        Style.Colors[ImGuiCol_TableRowBg] = BackroundTransparentColor;
        Style.Colors[ImGuiCol_TableRowBgAlt] = BackroundChildTransparentColor;
        Style.Colors[ImGuiCol_TextLink] = Accent1Color;
        Style.Colors[ImGuiCol_TextSelectedBg] = Accent1Color;
        Style.Colors[ImGuiCol_TreeLines] = TextDimmedColor;
        Style.Colors[ImGuiCol_DragDropTarget] = Accent1Color;
        Style.Colors[ImGuiCol_DragDropTargetBg] = TransparentColor;
        Style.Colors[ImGuiCol_UnsavedMarker] = Accent1Color;
        Style.Colors[ImGuiCol_NavCursor] = Accent1Color;
        Style.Colors[ImGuiCol_NavWindowingHighlight] = Accent1Color;
        Style.Colors[ImGuiCol_NavWindowingDimBg] = BackroundDimmedColor;
        Style.Colors[ImGuiCol_ModalWindowDimBg] = BackroundDimmedColor;
    }
}
