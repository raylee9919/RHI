// Copyright Seong Woo Lee. All Rights Reserved.

#include "IO.h"

#include <fstream>

namespace Engine
{
    namespace IO
    {
        ENGINE_API uint64_t ReadEntireFile(const String& path, void* buffer)
        {
            std::ifstream file(path.c_str(), std::ios::binary | std::ios::ate);

            if (!file.is_open()) {
                return 0;
            }

            uint64_t size = (uint64_t)file.tellg();

            if (!buffer) {
                return size;
            }

            file.seekg(0);
            file.read(static_cast<char*>(buffer), size);

            return (uint64_t)file.gcount();
        }
    }
}
