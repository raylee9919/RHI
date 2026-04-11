// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/Core_Common.h"

#include "Shader/Shader.h"

#include <DirectX/Include/dxgiformat.h>

#include <DXC/Include/d3d12shader.h>
#include <DXC/Include/dxcapi.h>

struct IDxcLibrary;
struct IDxcCompiler3;
struct IDxcUtils;
struct ID3D12ShaderReflection;

namespace Engine 
{
    namespace DXIL
    {
        struct Compiler
        {
            IDxcCompiler3* m_compiler;
            IDxcUtils*     m_utils;
        };

        struct CompileResult
        {
            uint8_t* bytes;
            uint64_t length;
            ID3D12ShaderReflection* reflection;
        };


        ENGINE_API bool InitCompiler(Compiler* compiler);
        ENGINE_API void DeinitCompiler(Compiler* compiler);

        [[nodiscard]] ENGINE_API CompileResult CompileShader(Compiler* compiler, HLSL_Shader& hlsl, bool debug);


        [[nodiscard]] ENGINE_API DXGI_FORMAT ToDXGIFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask);
    }
}
