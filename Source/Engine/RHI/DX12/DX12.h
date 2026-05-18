// Copyright Seong Woo Lee. All Rights Reserved.

#pragma once

#include "Core/SE_Basics.h"

#include <ThirdParty/DirectX/Include/d3d12.h>
#include <ThirdParty/DirectX/Include/d3dx12/d3dx12.h>

#include <dxgi1_6.h>

#if BUILD_DEBUG
#  include <dxgidebug.h>
#endif


#define DX12_MAX_NUM_FRAMES 3

namespace Engine
{
    struct DX12_Resource;

    struct DX12_Device {
        ID3D12Device10* native_device;
        IDXGIFactory6*  factory;
    };

    struct DX12_Fence {
        ID3D12Fence* native_fence;
        uint64_t     value;
        HANDLE       event;
    };

    struct DX12_Descriptor_Heap {
        ID3D12DescriptorHeap*         native_heap;
        D3D12_DESCRIPTOR_HEAP_TYPE    type;
        uint64_t                      descriptor_size;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        uint32_t                      max;
        uint32_t                      num;
        uint64_t*                     free_list; // bit fields.
        uint32_t                      num_nodes;
    };

    struct DX12_Descriptor {
        DX12_Descriptor_Heap*         my_heap;
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        u32                           index;
    };

    struct DX12_Graphics_Pipeline_Desc {
        ID3D12RootSignature*          root_signature;

        u32                           num_input_elements;
        D3D12_INPUT_ELEMENT_DESC*     input_elements;

        D3D12_PRIMITIVE_TOPOLOGY_TYPE topology;

        D3D12_CULL_MODE               cull_mode;

        bool                          depth_enabled;
        u32                           num_render_targets;
        DXGI_FORMAT                   rtv_formats[8];
        DXGI_FORMAT                   dsv_format;

        void*                         vs_bytecode;
        u64                           vs_length;

        void*                         ps_bytecode;
        u64                           ps_length;
    };

    struct DX12_Compute_Pipeline_Desc {
        ID3D12RootSignature*          root_signature;
        void*                         bytecode;
        u64                           length;
        u32                           thread_group_size[3];
    };

    enum DX12_Pipeline_Type {
        DX12_PIPELINE_TYPE_INVALID = 0,
        DX12_PIPELINE_TYPE_GRAPHICS,
        DX12_PIPELINE_TYPE_COMPUTE,
    };

    struct DX12_Pipeline_State {
        DX12_Pipeline_Type type  = DX12_PIPELINE_TYPE_INVALID;
        ID3D12PipelineState* pso = nullptr;
        union {
            DX12_Graphics_Pipeline_Desc graphics;
            DX12_Compute_Pipeline_Desc  compute;
        };
    };

    struct ENGINE_API DX12_Command_List {
        ID3D12GraphicsCommandList7* native_cmd_list;
        ID3D12CommandAllocator*     native_cmd_allocator;


        void begin();
        void end();
        void clear_rtv(DX12_Descriptor* descriptor, float r, float g, float b, float a);
        void clear_dsv(DX12_Descriptor* descriptor, float depth, int top_left_x, int top_left_y, int width, int height);
        void transition_barrier(DX12_Resource* resource, uint32_t subresource, D3D12_RESOURCE_STATES after);
        void set_pipeline_state(DX12_Pipeline_State* state);
        void set_resource_and_sampler_heap(DX12_Descriptor_Heap* resource_heap, DX12_Descriptor_Heap* sampler_heap);
        void set_graphics_root_signature(ID3D12RootSignature* root_signature);
        void set_graphics_root_constants(u32 root_parameter_index, u32 count, void* data);
        void set_viewport(int top_left_x, int top_left_y, int width, int height);
        void set_scissor(int top_left_x, int top_left_y, int width, int height);
        void set_topology(D3D12_PRIMITIVE_TOPOLOGY topology);
        void set_render_target(u32 num_rtvs, DX12_Descriptor* rtvs, DX12_Descriptor* dsv);
        void set_index_buffer(DX12_Resource* resource);
        void draw(u32 num_vertices, u32 num_instances, u32 begin_vertex = 0, u32 begin_instance = 0);
        void draw_indexed(u32 num_indices_per_instance, u32 num_instances, u32 start_index = 0, int base_vertex = 0, u32 start_instance = 0);

        void set_compute_root_signature(ID3D12RootSignature* root_signature);
        void set_compute_root_constants(u32 root_parameter_index, u32 count, void* data);
        void dispatch(u32 x, u32 y, u32 z);
    };

    struct DX12_Command_Queue {
        ID3D12CommandQueue* native_cmd_queue;
    };

    enum DX12_Resource_Type : int8_t {
        DX12_RESOURCE_TYPE_INVALID = -1,
        DX12_RESOURCE_TYPE_BUFFER,
        DX12_RESOURCE_TYPE_TEXTURE_2D,
    };

    struct DX12_Resource_Desc_Buffer {
        uint64_t size;
    };

    struct DX12_Resource_Desc_Texture {
        DXGI_FORMAT format;
        u32 width;
        u32 height;
        u16 mip_levels;
        u16 depth;
        u32 num_samples;
    };

    struct DX12_Resource_Desc {
        DX12_Resource_Type      type;

        D3D12_HEAP_TYPE         heap_type         = D3D12_HEAP_TYPE_DEFAULT;
        D3D12_CPU_PAGE_PROPERTY cpu_page_property = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        D3D12_HEAP_FLAGS        heap_flags        = D3D12_HEAP_FLAG_NONE;
        D3D12_RESOURCE_FLAGS    resource_flags    = D3D12_RESOURCE_FLAG_NONE;

        union {
            DX12_Resource_Desc_Buffer  buffer;
            DX12_Resource_Desc_Texture texture;
        };

        bool                    do_clear = false;
        D3D12_CLEAR_VALUE       clear_value = {};
    };

    struct DX12_Resource {
        DX12_Resource_Desc    desc;
        ID3D12Resource*       native_resource;
        D3D12_RESOURCE_STATES current_state;
    };

    struct ENGINE_API DX12_Swap_Chain {
        IDXGISwapChain3* native_swap_chain;
        u32              num_frames;
        u32              current_frame_index;
        DX12_Resource*   resources;
        DX12_Descriptor* rtvs;

        FORCE_INLINE void present() {
            UINT sync_interval = 0;
            native_swap_chain->Present(sync_interval, 0); // @Temporary: flags?
            current_frame_index = native_swap_chain->GetCurrentBackBufferIndex();
        }

        FORCE_INLINE DX12_Resource* get_current_resource() {
            return resources + current_frame_index;
        }

        FORCE_INLINE DX12_Descriptor get_current_rtv() {
            return rtvs[current_frame_index];
        }
    };


    ENGINE_API DXGI_FORMAT dxgi_format_from_component_type_and_mask(D3D_REGISTER_COMPONENT_TYPE type, BYTE mask);

    [[nodiscard]] ENGINE_API bool dx12_init_device(DX12_Device* device, bool use_debug_layer);
    [[nodiscard]] ENGINE_API bool dx12_deinit_device(DX12_Device* device);

    ENGINE_API DX12_Swap_Chain* dx12_create_swap_chain(DX12_Device* device, DX12_Command_Queue* cmd_queue, DX12_Descriptor_Heap* rtv_heap, HWND hwnd, u32 width, u32 height, u32 num_frames);
    ENGINE_API void dx12_destroy_swap_chain(DX12_Swap_Chain* swap_chain);

    [[nodiscard]] ENGINE_API bool dx12_init_command_queue(DX12_Device* device, DX12_Command_Queue* cmd_queue, D3D12_COMMAND_LIST_TYPE type);
    [[nodiscard]] ENGINE_API bool dx12_deinit_command_queue(DX12_Command_Queue* cmd_queue);

    [[nodiscard]] ENGINE_API bool dx12_init_command_list(DX12_Device* device, DX12_Command_List* cmd_list, D3D12_COMMAND_LIST_TYPE type);
    [[nodiscard]] ENGINE_API bool dx12_deinit_command_list(DX12_Command_List* cmd_list);

    [[nodiscard]] ENGINE_API bool dx12_init_fence(DX12_Device* device, DX12_Fence* fence);
    [[nodiscard]] ENGINE_API bool dx12_deinit_fence(DX12_Fence* fence);

    ENGINE_API DX12_Descriptor_Heap* dx12_create_descriptor_heap(DX12_Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, u32 max_descriptors);
    ENGINE_API void dx12_destroy_descriptor_heap(DX12_Descriptor_Heap* heap);

    ENGINE_API DX12_Descriptor dx12_alloc_descriptor(DX12_Descriptor_Heap* heap);
    ENGINE_API void dx12_dealloc_descriptor(const DX12_Descriptor& descriptor);

    ENGINE_API void dx12_execute_command_list(DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list);

    ENGINE_API void dx12_signal_fence(DX12_Command_Queue* cmd_queue, DX12_Fence* fence);
    ENGINE_API void dx12_wait_fence(DX12_Fence* fence);

    ENGINE_API DX12_Resource* dx12_alloc_resource(DX12_Device* device, DX12_Resource_Desc desc, D3D12_RESOURCE_STATES init_state = D3D12_RESOURCE_STATE_COMMON);
    ENGINE_API void dx12_dealloc_resource(DX12_Resource* resource);
    ENGINE_API DX12_Resource dx12_resource_from_native_resource(ID3D12Resource* resource, D3D12_RESOURCE_STATES current_state);

    ENGINE_API ID3D12RootSignature* dx12_create_bindless_root_signature(DX12_Device* device);

    ENGINE_API DX12_Pipeline_State dx12_create_graphics_pipeline_state(DX12_Device* device, const DX12_Graphics_Pipeline_Desc& desc);
    ENGINE_API DX12_Pipeline_State dx12_create_compute_pipeline_state(DX12_Device* device, const DX12_Compute_Pipeline_Desc& desc);

    ENGINE_API D3D12_PRIMITIVE_TOPOLOGY dx12_to_primitive_topology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);


    ENGINE_API void dx12_create_rtv(DX12_Device* device, DX12_Resource* resource, DX12_Descriptor* descriptor, D3D12_RENDER_TARGET_VIEW_DESC desc);
    ENGINE_API void dx12_create_dsv(DX12_Device* device, DX12_Resource* resource, DX12_Descriptor* descriptor, DXGI_FORMAT format);
    ENGINE_API void dx12_create_srv(DX12_Device* device, DX12_Resource* resource, DX12_Descriptor* descriptor, DXGI_FORMAT view_format, u32 num_elements = 0, u32 stride_in_bytes = 0);
    ENGINE_API void dx12_create_uav(DX12_Device* device, DX12_Resource* resource, DX12_Descriptor* descriptor, DXGI_FORMAT view_format, u32 num_elements = 0, u32 stride_in_bytes = 0);

    ENGINE_API void dx12_upload_buffer(DX12_Device *device, DX12_Command_Queue* cmd_queue, DX12_Command_List* cmd_list, DX12_Fence* fence, DX12_Resource* resource, void* data, u64 size);
}
