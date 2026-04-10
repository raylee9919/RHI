// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/Core_String.h"
#include "Core/Core_Log.h"
#include "Core/Core_Array.h"

#include "DXIL_Compiler.h"

#include <DXC/Include/dxcapi.h>

namespace Engine
{
    namespace DXIL
    {
        ENGINE_API bool InitCompiler(Compiler* out_compiler)
        {
            if (out_compiler)
            {
                // 'Utils' replaces 'Library' in DXC world.
                IDxcLibrary* library   = nullptr;
                IDxcCompiler* compiler = nullptr;
                IDxcUtils* utils       = nullptr;


                HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
                CORE_ASSERT_SUCCEEDED(hr);

                hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
                CORE_ASSERT_SUCCEEDED(hr);

                hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
                CORE_ASSERT_SUCCEEDED(hr);


                out_compiler->m_library  = library;
                out_compiler->m_compiler = compiler;
                out_compiler->m_utils    = utils;

                Log("Initted DXIL compiler.");
                return true;
            }
            else
            {
                return false;
            }
        }

        ENGINE_API void DeinitCompiler(Compiler* compiler)
        {
            if (compiler)
            {
                SafeReleaseCOM(&compiler->m_utils);
                SafeReleaseCOM(&compiler->m_compiler);
                SafeReleaseCOM(&compiler->m_library);
            }
        }

        ENGINE_API DXIL_Bytecode CompileShader(Compiler* compiler, HLSL_Shader& hlsl, bool debug)
        {
            LPCVOID source_ptr = (LPCVOID)hlsl.source;
            UINT source_length = hlsl.length;

            IDxcBlobEncoding* source_blob  = nullptr;
            IDxcOperationResult* op_result = nullptr;
            IDxcBlobEncoding* error_blob   = nullptr;
            IDxcBlobUtf8* error_blob_utf8  = nullptr;
            IDxcBlob* result_blob          = nullptr;

            WCHAR entry_point_wcstr[512] = {};
            swprintf_s(entry_point_wcstr, 512, L"%hs", hlsl.entry.c_str());

            WCHAR target_profile_wcstr[512] = {};
            swprintf_s(target_profile_wcstr, 512, L"%hs", hlsl.target_profile.c_str());

            Array<LPCWCHAR> arguments;
            arguments.push_back(L"-WX");    // Treat warnings as errors
            arguments.push_back(L"-Zpr");   // Pack matrices in row-major order
            arguments.push_back(L"-O3");    // Optimization Level 3 (default)
            if (debug) {
                arguments.push_back(L"-Zi");
                arguments.push_back(L"-Qembed_debug");
            }


            // Create source blob.
            UINT code_page = 0; // CP_UTF8??
            HRESULT hr = compiler->m_utils->CreateBlob(source_ptr, source_length, code_page, &source_blob);
            CORE_ASSERT_SUCCEEDED(hr);

            // Compile source blob.
            hr = compiler->m_compiler->Compile(source_blob, L"NAME???", entry_point_wcstr, target_profile_wcstr, arguments.data(), arguments.size(), nullptr, 0, nullptr, &op_result);
            CORE_ASSERT_SUCCEEDED(hr);

            // Get errors.
            hr = op_result->GetErrorBuffer(&error_blob);
            CORE_ASSERT_SUCCEEDED(hr);

            if (error_blob && error_blob->GetBufferSize() > 0)
            {
                hr = error_blob->QueryInterface(IID_PPV_ARGS(&error_blob_utf8));
                CORE_ASSERT_SUCCEEDED(hr);
                Log("dxil shader compilation error: %s", error_blob_utf8->GetStringPointer());
            }

            // Check status.
            HRESULT status;
            hr = op_result->GetStatus(&status);
            CORE_ASSERT_SUCCEEDED(hr);
            if (FAILED(status))
            {
                CORE_ASSERT(!"Encountered failed status during DXIL compilation.");
            }

            // Get result blob.
            if (FAILED(op_result->GetResult(&result_blob)) || !result_blob)
            {
                CORE_ASSERT(!"No resulting shader blob was produced during DXI compilation.");
            }

            // Return compilation result.
            DXIL_Bytecode result = {};
            {
                uint64_t len = result_blob->GetBufferSize();
                result.length = len;
                result.bytes = new uint8_t[len]; // @Todo: Release!!!!!!
                memcpy(result.bytes, result_blob->GetBufferPointer(), len);
            }


            // Cleanup
            SafeReleaseCOM(&source_blob);
            SafeReleaseCOM(&op_result);
            SafeReleaseCOM(&error_blob);
            SafeReleaseCOM(&error_blob_utf8);
            SafeReleaseCOM(&result_blob);

            Log("DXIL compilation successful.");
            return result;
        }
    }
}
