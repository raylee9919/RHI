// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once


namespace Engine
{
    class RHI_CommandQueue;
    enum class RHI_CommandType;

    static int g_num_frames_in_flight = 3;

    class RHI_Device
    {
        public:
            RHI_Device() = default;
            ~RHI_Device() = default;

            virtual RHI_CommandQueue* CreateCommandQueue(RHI_CommandType type) = 0;

        private:
    };
}
