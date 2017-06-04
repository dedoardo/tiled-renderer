/* render_bucket.cpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
// Header
#include <camy/buckets.hpp>

// camy

namespace camy
{
    void DrawCall::make_default(uint32 vertex_count, uint32 vertex_offset)
    {
        type = DrawCall::Type::Default;
        info.default.vertex_count = vertex_count;
        info.default.vertex_offset = vertex_offset;
    }

    void DrawCall::make_indexed(uint32 index_count, uint32 index_offset, uint32 vertex_offset)
    {
        type = DrawCall::Type::Indexed;
        info.indexed.index_count = index_count;
        info.indexed.index_offset = index_offset;
        info.indexed.vertex_offset = vertex_offset;
    }

    void DrawCall::make_instanced(uint32 vertex_count,
                                  uint32 instance_count,
                                  uint32 vertex_offset,
                                  uint32 instance_offset)
    {
        type = DrawCall::Type::Instanced;
        info.instanced.vertex_count = vertex_count;
        info.instanced.instance_count = instance_count;
        info.instanced.vertex_offset = vertex_offset;
        info.instanced.instance_offset = instance_offset;
    }

    void DrawCall::make_indexed_instanced(uint32 index_count,
                                          uint32 instance_count,
                                          uint32 index_offset,
                                          uint32 vertex_offset,
                                          uint32 instance_offset)
    {
        type = DrawCall::Type::IndexedInstanced;
        info.indexed_instanced.index_count = index_count;
        info.indexed_instanced.instance_count = instance_count;
        info.indexed_instanced.index_offset = index_offset;
        info.indexed_instanced.vertex_offset = vertex_offset;
        info.indexed_instanced.instance_offset = instance_offset;
    }

    PipelineState PipelineState::default()
    {
        PipelineState ps;
        ps.render_targets[0].resource = API::rc().get_backbuffer_handle();
        ps.render_targets[0].clear_bits = Clear::Color;
        ps.render_targets[0].color = colors::azure;

        Surface& backbuffer = API::rc().get_backbuffer();
        ps.viewport.top = 0;
        ps.viewport.left = 0;
        ps.viewport.right = backbuffer.desc.width;
        ps.viewport.bottom = backbuffer.desc.height;
        ps.viewport.near = 0.001f;
        ps.viewport.far = 1.f;
        return ps;
    }

    void ParameterBlock::set(rsize slot, ShaderVariable var, void* data)
    {
        parameters[slot].var = var;
        parameters[slot].data = data;
    }

    void ParameterBlock::set(rsize slot, ShaderVariable var, HResource handle, uint16 view)
    {
        parameters[slot].var = var;
        parameters[slot].res.handle = handle;
        parameters[slot].res.view = view;
    }

    RenderBucket::RenderBucket() {}

    RenderBucket::~RenderBucket() {}

    void RenderBucket::begin(const PipelineState& pipeline_state,
                             SortMode sort,
                             Parameter* shared_parameters,
                             rsize num_shared_parameters)
    {
        m_sort = sort;
        m_pipeline_state = pipeline_state;
        m_items.clear();
        m_parameters.reset();
        m_parameter_blocks.clear();

        if (shared_parameters != nullptr && num_shared_parameters == 0 ||
            shared_parameters == nullptr && num_shared_parameters > 0)
        {
            CL_ERR("Invalid argument: shared_parameters & num_shared_parameters mismatch");
            return;
        }

        m_shared_parameters.clear();
        for (rsize i = 0; i < num_shared_parameters; ++i)
            m_shared_parameters.append(shared_parameters[i]);
    }

    CAMY_INLINE bool _cmp_op_greater(RenderItem::Key a, RenderItem::Key b) { return a > b; }

    CAMY_INLINE bool _cmp_op_lesser(RenderItem::Key a, RenderItem::Key b) { return a < b; }

    void RenderBucket::end()
    {
        rsize current_size = m_sorted_indices.count();
        rsize next_size = m_items.count();

        if (next_size > current_size)
        {
            for (rsize i = current_size; i < next_size; ++i)
                m_sorted_indices.append((uint32)i);
        }
        else if (next_size < current_size)
        {
            m_sorted_indices.resize(next_size);
            for (rsize i = 0; i < next_size; ++i)
                m_sorted_indices[i] = (uint32)i;
        }

        // Sorting
        // Temporal coherency going for the good old insertion sort
        // https://www.gamedev.net/topic/661114-temporal-coherence-and-render-queue-sorting/
        if (m_sort != SortMode::None)
        {
            bool (*cmp_fun)(RenderItem::Key, RenderItem::Key) = nullptr;
            if (m_sort == SortMode::Ascending)
                cmp_fun = _cmp_op_greater;
            else if (m_sort == SortMode::Descending)
                cmp_fun = _cmp_op_lesser;

            for (rsize i = 1; i < next_size; ++i)
            {
                rsize j = i;
                RenderItem::Key key_i = m_items[m_sorted_indices[i]].key;

                RenderItem::Key key_jm1;
                while (j > 0 && cmp_fun((key_jm1 = m_items[m_sorted_indices[j - 1]].key), key_i))
                {
                    m_sorted_indices[j] ^= m_sorted_indices[j - 1];
                    m_sorted_indices[j - 1] ^= m_sorted_indices[j];
                    m_sorted_indices[j] ^= m_sorted_indices[j - 1];
                    --j;
                }
            }
        }

        _compile();
    }

    RenderItem& RenderBucket::next() { return m_items.next(); }

    ParameterBlock RenderBucket::next_block(rsize num_parameters, ParameterBlock::CacheTag tag)
    {
        ParameterBlock ret;
        ret.parameters = m_parameters.next_array(num_parameters);
        ret.num_parameters = num_parameters;
        ret.cache_tag = tag;
        return ret;
    }

    HParameterBlock RenderBucket::next_block_handle(const ParameterBlock& block)
    {
        HParameterBlock ret = m_parameter_blocks.count();
        m_parameter_blocks.next() = block;
        return ret;
    }

    CommandList& RenderBucket::to_command_list() { return m_command_list; }

    bool RenderBucket::is_ready() const
    {
        // TODO: IMPLEMENT
        return true;
    }

    void _set_pipeline_state(CommandList& command_list, const PipelineState& pipeline_state)
    {
        HResource render_targets[API::MAX_RENDER_TARGETS];
        for (rsize i = 0; i < API::MAX_RENDER_TARGETS; ++i)
            render_targets[i] = pipeline_state.render_targets[i].resource;

        command_list.set_targets(render_targets, API::MAX_RENDER_TARGETS,
                                 pipeline_state.depth_buffer.resource);
        command_list.set_viewport(pipeline_state.viewport);
        command_list.set_rasterizer_state(pipeline_state.rasterizer_state);
        // command_list.set_blend_state(pipeline_state.blend_state);
        command_list.set_depth_stencil_state(pipeline_state.depth_stencil_state);

        for (rsize i = 0; i < API::MAX_RENDER_TARGETS; ++i)
        {
            const PipelineState::Target& target = pipeline_state.render_targets[i];
            if (target.resource.is_valid())
            {
                if (target.clear_bits & Clear::Color)
                    command_list.clear_color(target.resource, target.color);
                if (target.clear_bits & Clear::Depth)
                    command_list.clear_depth(target.resource, target.depth);
                if (target.clear_bits & Clear::Stencil)
                    command_list.clear_stencil(target.resource, target.stencil);
            }
        }
    }

#pragma pack(push, 1)
    struct PipelineStateCache
    {
        HResource vertex_shader;
        HResource geometry_shader;
        HResource pixel_shader;
        HResource vertex_buffers[API::MAX_VERTEX_BUFFERS];
        HResource index_buffer;
        ParameterBlock::CacheTag cache_tags[API::MAX_PARAMETER_BLOCKS]{
            ParameterBlock::kInvalidCacheTag, ParameterBlock::kInvalidCacheTag,
            ParameterBlock::kInvalidCacheTag, ParameterBlock::kInvalidCacheTag,
            ParameterBlock::kInvalidCacheTag};
    };
#pragma pack(pop)

    // Draw lookup table
    static_assert((uint32)DrawCall::Type::Default == 0, "CommandList ftable error");
    inline void _draw_default(CommandList& command_list, const DrawCall& dc)
    {
        command_list.draw(dc.info.default.vertex_count, dc.info.default.vertex_offset);
    }

    static_assert((uint32)DrawCall::Type::Indexed == 1, "CommandList ftable error");
    inline void _draw_indexed(CommandList& command_list, const DrawCall& dc)
    {
        command_list.draw_indexed(dc.info.indexed.index_count, dc.info.indexed.index_offset,
                                  dc.info.indexed.vertex_offset);
    }

    static_assert((uint32)DrawCall::Type::Instanced == 2, "CommandList ftable error");
    inline void _draw_instanced(CommandList& command_list, const DrawCall& dc)
    {
        command_list.draw_instanced(
            dc.info.instanced.vertex_count, dc.info.instanced.instance_count,
            dc.info.instanced.vertex_offset, dc.info.instanced.instance_offset);
    }

    static_assert((uint32)DrawCall::Type::IndexedInstanced == 3, "CommandList ftable error");
    inline void _draw_indexed_instanced(CommandList& command_list, const DrawCall& dc)
    {
        command_list.draw_indexed_instanced(
            dc.info.indexed_instanced.index_count, dc.info.indexed_instanced.instance_count,
            dc.info.indexed_instanced.index_offset, dc.info.indexed_instanced.vertex_offset,
            dc.info.indexed_instanced.instance_offset);
    }

    void (*_draw_ftable[])(CommandList& command_list, const DrawCall& dc){
        _draw_default, _draw_indexed, _draw_instanced, _draw_indexed_instanced};

    void RenderBucket::_compile()
    {
        // Resetting incremental allocators
        for (rsize i = 0; i < m_uploads.count(); ++i)
            m_uploads[i].cur = 0;
        m_current_upload = -1;

        m_command_list.begin();

        // Pipeline State
        _set_pipeline_state(m_command_list, m_pipeline_state);

        // Shared state
        for (rsize i = 0; i < m_shared_parameters.count(); ++i)
        {
            Parameter& param = m_shared_parameters[i];
            if (!param.var.valid())
            {
                CL_ERR("Trying to set invalid *shared* parameter, likely a call to Program::var() "
                       "failed somewhere");
                continue;
            }

            if ((BindType)param.var.type() == BindType::Constant)
            {
                CL_ERR("Can't set individual constants: ", param.var);
                continue;
            }

            if ((BindType)param.var.type() != BindType::ConstantBuffer)
                m_command_list.set_parameter(param.var, param.res.handle, (uint8)param.res.view);
            else
            {
                HResource handle;
                rsize offset;
                _incr_alloc(param, handle, offset);
                m_command_list.set_cbuffer_off(param.var, handle, offset);
            }
        }

        m_command_list.set_primitive_topology(PrimitiveTopology::TriangleList);

        // Items
        PipelineStateCache cache;
        for (rsize i = 0; i < m_sorted_indices.count(); ++i)
        {
            RenderItem& item = m_items[m_sorted_indices[i]];

            m_command_list.set_input_signature(item.input_signature);

            static_assert(API::MAX_VERTEX_BUFFERS == 2 && sizeof(HResource) == 2,
                          "Code below relies on this assumption");
            if (*(uint32*)cache.vertex_buffers != *(uint32*)item.vertex_buffers)
            {
                m_command_list.set_vertex_buffers(0, item.vertex_buffers, API::MAX_VERTEX_BUFFERS);
                *(uint32*)cache.vertex_buffers = *(uint32*)item.vertex_buffers;
            }

            if (cache.index_buffer != item.index_buffer)
            {
                m_command_list.set_index_buffer(item.index_buffer);
                cache.index_buffer = item.index_buffer;
            }

            if (cache.vertex_shader != item.vertex_shader)
            {
                m_command_list.set_vertex_shader(item.vertex_shader);
                cache.vertex_shader = item.vertex_shader;
            }

            if (cache.geometry_shader != item.geometry_shader)
            {
                m_command_list.set_geometry_shader(item.geometry_shader);
                cache.geometry_shader = item.geometry_shader;
            }

            if (cache.pixel_shader != item.pixel_shader)
            {
                m_command_list.set_pixel_shader(item.pixel_shader);
                cache.pixel_shader = item.pixel_shader;
            }

            for (rsize b = 0; b < API::MAX_PARAMETER_BLOCKS; ++b)
            {
                if (item.parameters[b].is_valid() == false) continue;

                ParameterBlock& block = m_parameter_blocks[item.parameters[b]];
                if (block.cache_tag == ParameterBlock::kInvalidCacheTag ||
                    block.cache_tag != cache.cache_tags[b])
                {
                    for (rsize p = 0; p < block.num_parameters; ++p)
                    {
                        Parameter& param = block.parameters[p];
                        if (!param.var.valid())
                        {
                            CL_ERR("Trying to set invalid parameter, likely a call to "
                                   "Program::var() failed somewhere");
                            continue;
                        }

                        if ((BindType)param.var.type() == BindType::Constant)
                        {
                            CL_ERR("Can't set individual constants: ", param.var);
                            continue;
                        }

                        if ((BindType)param.var.type() != BindType::ConstantBuffer)
                            m_command_list.set_parameter(param.var, param.res.handle,
                                                         (uint8)param.res.view);
                        else
                        {
                            HResource handle;
                            rsize offset;
                            _incr_alloc(param, handle, offset);
                            m_command_list.set_cbuffer_off(param.var, handle, offset);
                        }
                    }

                    cache.cache_tags[b] = block.cache_tag;
                }
            }

            if (item.draw_call.type != DrawCall::Type::None)
                _draw_ftable[(rsize)item.draw_call.type](m_command_list, item.draw_call);
        }

        for (int i = 0; i <= m_current_upload; ++i)
        {
            ConstantBufferUpload& upload = m_uploads[i];
            m_command_list.queue_constant_buffer_update(
                upload.handle, upload.data, upload.cur * API::query(API::Query::ConstantByteSize));
        }

        m_command_list.end();
    }

    void
    RenderBucket::_incr_alloc(const Parameter& parameter, HResource& handle_out, rsize& offset_out)
    {
        rsize required = parameter.var.size();
        CAMY_ASSERT(required % API::query(API::Query::ConstantByteSize) == 0);
        required /= API::query(API::Query::ConstantByteSize);

        // Honestly, this is merely playing w/ indices
        // This way we signal that atleast one cbuffer update should be queued
        if (m_current_upload == -1) m_current_upload = 0;

        // Are we out of space in the cbuffer currently being used and can we reuse a previous
        // cbuffer
        if (m_current_upload < ((int)m_uploads.count() - 1) &&
            required > (m_uploads[m_current_upload].count - m_uploads[m_current_upload].cur))
            ++m_current_upload;

        // We are already at the last cbuffer and there is no space
        else if (m_current_upload >= ((int)m_uploads.count() - 1))
        {
            // do we have space ?
            if (m_uploads.empty() ||
                required > (m_uploads[m_current_upload].count - m_uploads[m_current_upload].cur))
            {
                m_uploads.emplace_last();
                ConstantBufferUpload& upload = m_uploads.last();

                ConstantBufferDesc cbd;
                cbd.size = 1024 * API::query(API::Query::ConstantByteSize);
                upload.handle =
                    API::rc().create_constant_buffer(cbd, "Incremental ConstantBuffer1 __internal");
                upload.data = (byte*)allocate(CAMY_UALLOC(cbd.size));
                upload.count = 1024;
                upload.cur = 0;
                m_current_upload = m_uploads.count() - 1;
            }
        }

        // Else there is space in the current cbuffer
        ConstantBufferUpload& upload = m_uploads[m_current_upload];
        std::memcpy(upload.data + upload.cur * API::query(API::Query::ConstantByteSize),
                    parameter.data, required * API::query(API::Query::ConstantByteSize));
        handle_out = upload.handle;
        offset_out = upload.cur;
        upload.cur += required;
    }
}