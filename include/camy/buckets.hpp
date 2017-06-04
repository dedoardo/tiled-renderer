/* buckets.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/command_list.hpp>
#include <camy/containers/dyn_linear_array.hpp>
#include <camy/containers/paged_linear_vector.hpp>
#include <camy/containers/dyn_array.hpp>
#include <camy/core.hpp>
#include <camy/graphics.hpp>
#include <camy/render_context.hpp>

namespace camy
{
    class CommandList;

    class CAMY_API IBucket
    {
      public:
        virtual ~IBucket() = default;

        // compiled version of the bucket
        virtual CommandList& to_command_list() = 0;
        virtual bool is_ready() const = 0;
    };

#pragma pack(push, 1)
    //! Draw call
    //! see make_***
    struct CAMY_API DrawCall
    {
        enum class Type : uint16
        {
            Default = 0,
            Indexed,
            Instanced,
            IndexedInstanced,
            Count,
            None
        };

        struct Default
        {
            uint32 vertex_count = 0;
            uint32 vertex_offset = 0;
        };

        struct Indexed
        {
            uint32 index_count = 0;
            uint32 index_offset = 0;
            uint32 vertex_offset = 0;
        };

        struct Instanced
        {
            uint32 vertex_count = 0; // (per instance)
            uint32 instance_count = 0;
            uint32 vertex_offset = 0;
            uint32 instance_offset = 0;
        };

        struct IndexedInstanced
        {
            uint32 index_count = 0; // (per instance)
            uint32 instance_count;
            uint32 index_offset;
            uint32 vertex_offset;
            uint32 instance_offset;
        };

        union Info {
            Info(){}; // uff....

            Default default;
            Indexed indexed;
            Instanced instanced;
            IndexedInstanced indexed_instanced;
        };

        void make_default(uint32 vertex_count, uint32 vertex_offset);
        void make_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset);
        void make_instanced(uint32 vertex_count,
                            uint32 instance_count,
                            uint32 vertex_offset,
                            uint32 instance_offset);
        void make_indexed_instanced(uint32 index_count,
                                    uint32 instance_count,
                                    uint32 index_offset,
                                    uint32 vertex_offset,
                                    uint32 instance_offset);

        Type type = Type::None;
        Info info;
    };
    static_assert(sizeof(DrawCall) == 22,
                  "Drawcall not packed as expected. This assert can be "
                  "removed even though it might be worth investigating the "
                  "error");
#pragma pack(pop)

    CAMY_ENUM_BITS(Clear, uint32){Color = 1, Depth = 1 << 1, Stencil = 1 << 2};

    struct CAMY_API PipelineState
    {
        struct Target
        {
            HResource resource;
            ClearBits clear_bits;
            float4 color;
            float depth;
            float stencil;
        };

        Target render_targets[API::MAX_RENDER_TARGETS];
        Target depth_buffer;
        HResource rasterizer_state;
        HResource blend_state;
        HResource depth_stencil_state;
        Viewport viewport;

        static PipelineState default();
    };

#pragma pack(push, 1)
    struct CAMY_API Parameter
    {
        //! Need to define it because of the union
        Parameter() { data = nullptr; }

        struct ParameterResource
        {
            HResource handle;
            uint16 view;
        };

        union {
            const void* data;
            ParameterResource res;
        };
        ShaderVariable var = ShaderVariable::make_invalid();
    };
    static_assert(sizeof(Parameter) == sizeof(void*) + 4,
                  "Parameter not packed as expected. This "
                  "assert can be removed even though it "
                  "might be worth investigating the error");
#pragma pack(pop)

// Caching mechanism for group of parameters.
// Parameters are organized in a cache-like way
// the SET is the index in RenderItem::parameters[SET]
// and the tag is the one in ParameterBlock (TODO: merge num_parameters and cache_id and make it 24
// bits)
// if by any reason two successive blocks at the same index (SET) match tag
// then no binding/uploading occurs. Remember this is local to a single bucket.
// An example would be a bunch of textures associate w/ a material. Choose a slot for the block
// and everytime you set a material generate an ID based on the material. You can use the
// same one used for sorting for instance and cram it into 16-bits. There is nothing to lose
// while having a possible speed increase.
#pragma pack(push, 1)
    struct CAMY_API ParameterBlock
    {
        using CacheTag = uint16;
        static const CacheTag kInvalidCacheTag = (CacheTag)-1;

        Parameter* parameters = nullptr;
        uint16 num_parameters = 0;
        CacheTag cache_tag = kInvalidCacheTag;

        void set(rsize slot, ShaderVariable var, void* data);
        void set(rsize slot, ShaderVariable var, HResource handle, uint16 view = 0);
    };
    static_assert(sizeof(ParameterBlock) == sizeof(void*) + 4,
                  "ParameterBlock not packed as expected. This assert can be removed even though "
                  "it might be worth investigating the error");
#pragma pack(pop)

#pragma pack(push, 1)
    struct CAMY_API HParameterBlock
    {
        uint32 _v = (uint32)-1;

        HParameterBlock(uint32 val = -1) { _v = val; }
        operator uint32() { return _v; }
        bool is_valid() const { return _v != (uint32)-1; }
        bool is_invalid() const { return _v == (uint32)-1; }

        constexpr static uint32 make_invalid() { return (uint32)-1; }
    };
#pragma pack(pop)

#pragma pack(push, 1)
    struct CAMY_API RenderItem
    {
        using Key = uint64;

        Key key = (Key)-1;
        HResource vertex_buffers[API::MAX_VERTEX_BUFFERS];
        HResource index_buffer;
        HResource vertex_shader;
        HResource pixel_shader;
        HResource geometry_shader;
        HResource input_signature;
        HParameterBlock parameters[API::MAX_PARAMETER_BLOCKS];
        DrawCall draw_call;
    };
#pragma pack(pop)
    static_assert(sizeof(RenderItem) == 64,
                  "RenderItem not packed as expected. This assert can be "
                  "removed even though it might be worth inverstigating "
                  "the error");

    //! <RenderBucket>
    //! Manages and sorts RenderItems, every RenderItem
    class CAMY_API RenderBucket final : public IBucket
    {
      public:
        enum SortMode
        {
            Ascending,
            Descending,
            None
        };

      public:
        RenderBucket();
        ~RenderBucket();

        //! Initial parameters can be temporary indeed
        void begin(const PipelineState& pipeline_state,
                   SortMode sort,
                   Parameter* shared_parameters = nullptr,
                   rsize num_shared_parameters = 0);

        //! This is the very function that you want to multithread as it sorts
        //! and compiles command into the command list that can be then flushed
        //! to the GPU
        void end();

        RenderItem& next();
        ParameterBlock next_block(rsize num_parameters,
                                  ParameterBlock::CacheTag tag = ParameterBlock::kInvalidCacheTag);
        HParameterBlock next_block_handle(const ParameterBlock& block);

        CommandList& to_command_list() override;
        bool is_ready() const override;

      private:
        void _compile();
        void _incr_alloc(const Parameter& parameter, HResource& handle, rsize& offset);

        CommandList m_command_list;
        DynLinearArray<RenderItem> m_items;
        DynLinearArray<ParameterBlock> m_parameter_blocks;
        PagedLinearVector<Parameter> m_parameters;
        DynArray<uint32> m_sorted_indices;

        SortMode m_sort;
        PipelineState m_pipeline_state;
        DynArray<Parameter> m_shared_parameters;

        struct ConstantBufferUpload
        {
            HResource handle;
            byte* data = nullptr;
            rsize count = 0;
            rsize cur = 0;
        };
        DynArray<ConstantBufferUpload> m_uploads;
        int m_current_upload;
    };
}