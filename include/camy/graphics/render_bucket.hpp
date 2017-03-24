/* render_bucket.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/core/base.hpp>
#include <camy/graphics/base.hpp>
#include <camy/graphics/ibucket.hpp>
#include <camy/graphics/command_list.hpp>
#include <camy/core/memory/vector.hpp>
#include <camy/core/memory/linear_vector.hpp>
#include <camy/core/memory/paged_linear_vector.hpp>

namespace camy
{
#pragma pack(push, 1)
    struct camy_api DrawCall
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
    
        union Info
        {
            Info() { }; // uff.... 

            Default default;
            Indexed indexed;
            Instanced instanced;
            IndexedInstanced indexed_instanced;
        };

        void set_default(uint32 vertex_count, uint32 vertex_offset);
        void set_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset);
        void set_instanced(uint32 vertex_count, uint32 instance_count, uint32 vertex_offset, uint32 instance_offset);
        void set_indexed_instanced(uint32 index_count, uint32 instance_count, uint32 index_offset, uint32 vertex_offset, uint32 instance_offset);

		Type type = Type::None;
        Info info;
    };

    struct PipelineState
    {
        HResource render_targets[kMaxBindableRenderTargets]{ kInvalidHResource, kInvalidHResource };
        HResource depth_buffer = kInvalidHResource;
        HResource rasterizer_state = kInvalidHResource;
        HResource blend_state = kInvalidHResource;
        Viewport  viewport;

        static PipelineState default();
    };

#pragma pack(pop)
    static_assert(sizeof(DrawCall) == 22, "Drawcall not packed as expected. This assert can be removed even though it might be worth investigating the error");

    struct Parameter
    {
        union
        {
            const void*     data;
            HResource handle;
        };
        ShaderVariable var;
    };
    static_assert(sizeof(Parameter) == 16, "Parameter not packed as expected. This assert can be removed even though it might be worth investigating the error");

    // Caching mechanism for group of parameters.
    // Parameters are organized in a cache-like way
    // the SET is the index in RenderItem::parameters[SET]
    // and the tag is the one in ParameterBlock (TODO: merge num_parameters and cache_id and make it 24 bits)
    // if by any reason two successive blocks at the same index (SET) match tag
    // then no binding/uploading occurs. Remember this is local to a single bucket.
    // An example would be a bunch of textures associate w/ a material. Choose a slot for the block
    // and everytime you set a material generate an ID based on the material. You can use the 
    // same one used for sorting for instance and cram it into 16-bits. There is nothing to lose
    // while having a possible speed increase.
    struct ParameterBlock
    {
        using CacheTag = uint16;
        static const CacheTag kInvalidCacheTag = (CacheTag)-1;

        Parameter* parameters = nullptr;
        uint16     num_parameters = 0;
        CacheTag   cache_tag = kInvalidCacheTag;

		void set(rsize slot, ShaderVariable var, void* data);
		void set(rsize slot, ShaderVariable var, HResource handle);
	};
    static_assert(sizeof(ParameterBlock) == 16, "ParameterBlock not packed as expected. This assert can be removed even though it might be worth investigating the error");

    using HParameterBlock = uint32;
    const HParameterBlock kInvalidHParameterBlock = (HParameterBlock)-1;
    using HResource = uint16;

#pragma pack(push, 1)
    struct camy_api RenderItem
    {
 //       RenderItem() = default;
        using Key = uint64;
        Key key = (Key)-1;
        HResource vertex_buffers[kMaxBindableVertexBuffers]{ kInvalidHResource, kInvalidHResource };
        HResource index_buffer = kInvalidHResource;
        HResource vertex_shader = kInvalidHResource;
        HResource pixel_shader = kInvalidHResource;
        HResource geometry_shader = kInvalidHResource;
        HResource input_signature = kInvalidHResource;
        HParameterBlock parameters[kMaxParameterBlocks] { kInvalidHParameterBlock, kInvalidHParameterBlock, kInvalidHParameterBlock, kInvalidHParameterBlock, kInvalidHParameterBlock };
        DrawCall draw_call;
    };
#pragma pack(pop)
    static_assert(sizeof(RenderItem) == 64, "RenderItem not packed as expected. This assert can be removed even though it might be worth inverstigating the error");

    class camy_api RenderBucket final : public IBucket
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

        void begin(const PipelineState& pipeline_state, SortMode sort);
        void end();

        RenderItem&     next();
        ParameterBlock  next_block(rsize num_parameters, ParameterBlock::CacheTag tag = ParameterBlock::kInvalidCacheTag);
        HParameterBlock next_block_handle(const ParameterBlock& block);

        CommandList& to_command_list()override;
        bool is_ready()const override;
    private:
        void _compile();
        void _incr_alloc(const Parameter& parameter, HResource& handle, rsize& offset);

        CommandList m_command_list;
        LinearVector<RenderItem> m_items;
        LinearVector<ParameterBlock> m_parameter_blocks;
        PagedLinearVector<Parameter> m_parameters;
        Vector<uint32> m_sorted_indices;
    
        SortMode       m_sort;
        PipelineState m_pipeline_state;

        struct ConstantBufferUpload
        {
            HResource handle = kInvalidHResource;
            byte*     data = nullptr;
            rsize     count = 0;
            rsize     cur = 0;
        };
        Vector<ConstantBufferUpload> m_uploads;
        rsize                        m_current_upload;
    };
}