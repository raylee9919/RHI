// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

#include "Shader/Shader.h"

struct IDxcLibrary;
struct IDxcCompiler;
struct IDxcUtils;

namespace Engine 
{
    namespace DXIL
    {
        struct Compiler
        {
            IDxcLibrary*   m_library;
            IDxcCompiler*  m_compiler;
            IDxcUtils*     m_utils;
        };

        struct DXIL_Bytecode
        {
            uint8_t* bytes;
            uint64_t length;
        };


        ENGINE_API bool InitCompiler(Compiler* compiler);
        ENGINE_API void DeinitCompiler(Compiler* compiler);

        [[nodiscard]] ENGINE_API DXIL_Bytecode CompileShader(Compiler* compiler, HLSL_Shader& hlsl, bool debug);
    }
}
