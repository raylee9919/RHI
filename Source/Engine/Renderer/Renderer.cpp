// Copyright Seong Woo Lee. All Rights Reserved.

#include "Renderer.h"

#include "RHI/DX12/RHI_DX12.h"

#include "Input/Input.h"

namespace Engine
{
    using namespace DX12;

    namespace Render
    {
        void Camera::Update(f32 dt, Input_System* input)
        {
            vec3 forward = vec3(0.f, 0.f, 1.f);
            {
                vec4 f = YRotation(yaw) * XRotation(pitch) * vec4(forward, 0.f);
                //forward.x = cos(pitch) * cos(yaw);
                //forward.y = sin(pitch);
                //forward.z = cos(pitch) * sin(yaw);
                forward = Normalize(f.xyz);
            }

            vec3 right = Cross(forward, vec3(0.f, 1.f, 0.f));

            f32 move_speed = speed;
            if (input->key_is_down[KEY_LEFT_SHIFT])
            {
                move_speed *= 3.0f;
            }

            f32 factor = dt * move_speed;

            if (input->key_is_down[KEY_E])
            {
                position += ( vec4(0.f, 1.f, 0.f, 0.f) * factor );
            }

            if (input->key_is_down[KEY_Q])
            {
                position -= ( vec4(0.f, 1.f, 0.f, 0.f) * factor );
            }

            if (input->key_is_down[KEY_W])
            {
                position += ( vec4(forward, 0.f) * factor );
            }

            if (input->key_is_down[KEY_S])
            {
                position -= ( vec4(forward, 0.f) * factor );
            }

            if (input->key_is_down[KEY_D])
            {
                position += ( vec4(right, 0.f) * factor );
            }

            if (input->key_is_down[KEY_A])
            {
                position -= ( vec4(right, 0.f) * factor );
            }

            if (input->mouse_is_down[MOUSE_LEFT])
            {
                if (!input->mouse_was_down[MOUSE_LEFT])
                {
                    last_mouse_x = input->current_mouse_x;
                    last_mouse_y = input->current_mouse_y;
                }
                else
                {
                    f32 dx = input->current_mouse_x - last_mouse_x;
                    f32 dy = input->current_mouse_y - last_mouse_y;

                    f32 rot_speed = 0.125f;
                    yaw   += (rot_speed * dx * dt);
                    pitch -= (rot_speed * dy * dt);
                    yaw   = FMod(yaw, PI * 2.f);
                    pitch = Clamp(pitch, -PI * 0.45f, PI * 0.45f);

                    last_mouse_x = input->current_mouse_x;
                    last_mouse_y = input->current_mouse_y;
                }
            }

            view      = m4x4::LookToLH(position.xyz, forward, vec3(0.f, 1.f, 0.f));
            proj      = m4x4::PerspectiveLH(DegreeToRadian(fov), aspect_ratio, near_z, far_z);
            view_proj = proj * view;
        }
    }
}
