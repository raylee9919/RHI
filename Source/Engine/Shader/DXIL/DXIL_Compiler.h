// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"

#include "Shader/Shader.h"

#include <DirectX/Include/dxgiformat.h>

#include <DXC/Include/d3d12shader.h>
#include <DXC/Include/dxcapi.h>

struct IDxcLibrary;
struct IDxcCompiler3;
struct IDxcUtils;
struct ID3D12ShaderReflection;
struct D3D12_INPUT_ELEMENT_DESC;

namespace Engine 
{
    namespace DXIL
    {
        struct ENGINE_API Compiled_Shader {
            IDxcResult* result;
            u8*         bytecode;
            u64         length;

            void release();
        };

        struct ENGINE_API Reflection {
            ID3D12ShaderReflection* reflection;
            D3D12_SHADER_DESC       shader_desc;

            void release();

            void get_input_parameters(D3D12_INPUT_ELEMENT_DESC* descs);
        };

        struct ENGINE_API Shader_Compiler {
            IDxcCompiler3* native_compiler;
            IDxcUtils*     utils;

            Compiled_Shader compile(bool debug, void* source, u64 size, const String& source_name, const char* entry, const char* target_profile);
            Reflection reflect(IDxcResult* compile_result);
        };

        ENGINE_API bool init_shader_compiler(Shader_Compiler* compiler);
        ENGINE_API void deinit_shader_compiler(Shader_Compiler* compiler);
    }


    namespace DXIL
    {
        struct Compiler
        {
            IDxcCompiler3* m_compiler;
            IDxcUtils*     m_utils;
        };

        struct CompiledShader
        {
            uint8_t* bytes;
            uint64_t length;
            ID3D12ShaderReflection* reflection;
        };


        ENGINE_API bool InitCompiler(Compiler* compiler);
        ENGINE_API void DeinitCompiler(Compiler* compiler);

        [[nodiscard]] ENGINE_API CompiledShader CompileShader(Compiler* compiler, bool debug, void* hlsl_source, uint64_t size, const String& entry, const String& target_profile);


        [[nodiscard]] ENGINE_API DXGI_FORMAT ToDXGIFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask);
    }
}
