// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

namespace Engine
{
    class RHI_CommandList;

    enum class RHI_CommandType
    {
        DIRECT,
        COMPUTE,
        COPY,
        VIDEO_ENCODE,
        VIDEO_DECODE,
    };

    class RHI_CommandQueue
    {
        public:
            RHI_CommandQueue() = default;
            ~RHI_CommandQueue() = default;

            virtual RHI_CommandList* CreateCommandList() = 0;

        protected:
            RHI_CommandType m_type;
    };
}
