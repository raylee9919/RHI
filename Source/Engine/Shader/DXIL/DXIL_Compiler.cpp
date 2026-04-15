// Copyright Seong Woo Lee. All Rights Reserved.

#include "Core/SE_String.h"
#include "Core/Core_Log.h"
#include "Core/Core_Array.h"

#include "DXIL_Compiler.h"


namespace Engine
{
    namespace DXIL
    {
        ENGINE_API bool InitCompiler(Compiler* out_compiler)
        {
            if (out_compiler)
            {
                // 'Utils' replaces 'Library' in DXC world.
                IDxcCompiler3* compiler = nullptr;
                IDxcUtils* utils       = nullptr;

                HRESULT hr = S_OK;

                hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
                CORE_ASSERT_SUCCEEDED(hr);

                hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils));
                CORE_ASSERT_SUCCEEDED(hr);


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
            }
        }

        ENGINE_API CompiledShader CompileShader(Compiler* compiler, bool debug, 
                                                void* hlsl_source, uint64_t size, 
                                                const String& entry, const String& target_profile)
        {
            DxcBuffer source_buffer = {
                .Ptr = hlsl_source,
                .Size = size,
                .Encoding = 0u
            };

            IDxcResult* compile_result               = nullptr;
            IDxcBlobUtf8* error_msgs                 = nullptr;
            IDxcBlob* shader_obj                     = nullptr;
            IDxcBlob* reflection                     = nullptr;
            ID3D12ShaderReflection* d3d12_reflection = nullptr;

            WCHAR entry_point_wcstr[512] = {};
            swprintf_s(entry_point_wcstr, 512, L"%hs", entry.c_str());

            WCHAR target_profile_wcstr[512] = {};
            swprintf_s(target_profile_wcstr, 512, L"%hs", target_profile.c_str());

            // Pack arguments.
            Array<LPCWCHAR> arguments;
            {
                arguments.push_back(L"-E");
                arguments.push_back(entry_point_wcstr);

                arguments.push_back(L"-T");
                arguments.push_back(target_profile_wcstr);

                arguments.push_back(L"-WX");    // Treat warnings as errors
                arguments.push_back(L"-Zpr");   // Pack matrices in row-major order
                if (debug) {
                    arguments.push_back(L"-Zi");
                    arguments.push_back(L"-Qembed_debug");
                    arguments.push_back(L"-Od");
                } else {
                    arguments.push_back(L"-O3"); // Optimization Level 3 (default)
                }
                arguments.push_back(DXC_ARG_ALL_RESOURCES_BOUND);
            }


            // Compile source blob.
            //
            UINT num_args = (UINT)arguments.size();
            HRESULT hr = compiler->m_compiler->Compile(&source_buffer, arguments.data(), num_args, nullptr, IID_PPV_ARGS(&compile_result));
            if (FAILED(hr))
            {
                return {};
            }

            // Check status and get errors.
            //
            compile_result->GetStatus(&hr);
            compile_result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&error_msgs), nullptr);

            if (error_msgs && error_msgs->GetStringLength())
            {
                Log("Compile returned HREUSLT (0x%x), error/warnings:\n\n %s\n", hr, error_msgs->GetStringPointer());
            }

            if (FAILED(hr))
            {
                return {};
            }

            // Get shader object.
            //
            hr = compile_result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader_obj), nullptr);
            if (FAILED(hr))
            {
                return {};
            }

            // Reflection
            //
            hr = compile_result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflection), nullptr);
            if (FAILED(hr))
            {
                return {};
            }

            DxcBuffer reflection_buffer = {
                .Ptr = reflection->GetBufferPointer(),
                .Size = reflection->GetBufferSize(),
                .Encoding = 0u,
            };

            hr = compiler->m_utils->CreateReflection(&reflection_buffer, IID_PPV_ARGS(&d3d12_reflection));
            if (FAILED(hr))
            {
                return {};
            }

            // Prepare struct that'll be returned.
            //
            CompiledShader result = {};
            {
                uint64_t len = shader_obj->GetBufferSize();
                result.length = len;
                result.bytes = new uint8_t[len]; // @Todo: Release
                memcpy(result.bytes, shader_obj->GetBufferPointer(), len);
                result.reflection = d3d12_reflection; // @Todo: Release
            }

            // Cleanup
            SafeReleaseCOM(&compile_result);
            SafeReleaseCOM(&error_msgs);
            SafeReleaseCOM(&shader_obj);
            SafeReleaseCOM(&reflection);

            Log("DXIL compilation successful.");
            return result;
        }

        ENGINE_API DXGI_FORMAT ToDXGIFormat(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask)
        {
            DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

            switch (type)
            {
                case D3D_REGISTER_COMPONENT_UNKNOWN:
                {
                    format = DXGI_FORMAT_UNKNOWN;
                } break;

                case D3D_REGISTER_COMPONENT_FLOAT32:
                {
                    if      (mask == 0x1) { format = DXGI_FORMAT_R32_FLOAT;          }
                    else if (mask == 0x3) { format = DXGI_FORMAT_R32G32_FLOAT;       }
                    else if (mask == 0x7) { format = DXGI_FORMAT_R32G32B32_FLOAT;    }
                    else if (mask == 0xf) { format = DXGI_FORMAT_R32G32B32A32_FLOAT; }
                } break;

                case D3D_REGISTER_COMPONENT_UINT32:
                {
                    if      (mask == 0x1) { format = DXGI_FORMAT_R32_UINT;            }
                    else if (mask == 0x3) { format = DXGI_FORMAT_R32G32_UINT;         }
                    else if (mask == 0x7) { format = DXGI_FORMAT_R32G32B32_UINT;      }
                    else if (mask == 0xf) { format = DXGI_FORMAT_R32G32B32A32_UINT;   }
                } break;

                case D3D_REGISTER_COMPONENT_SINT32:
                {
                    if      (mask == 0x1) { format = DXGI_FORMAT_R32_SINT;            }
                    else if (mask == 0x3) { format = DXGI_FORMAT_R32G32_SINT;         }
                    else if (mask == 0x7) { format = DXGI_FORMAT_R32G32B32_SINT;      }
                    else if (mask == 0xf) { format = DXGI_FORMAT_R32G32B32A32_SINT;   }
                } break;

                INVALID_DEFAULT_CASE;
            }

            return format;
        }
    }
}
