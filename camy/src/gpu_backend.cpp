// Header
#include <camy/gpu_backend.hpp>

// camy
#include <camy/error.hpp>
#include <camy/resources.hpp>
#include <camy/features.hpp>
#include <camy/layers.hpp>
#include <camy/pipeline_cache.hpp>
#include <camy/queue.hpp>

// shaders
#define BYTE camy::Byte
#include "shaders/pp_vs.hpp"
#undef BYTE

// C++ STL
#include <algorithm>

// D3D11 / DXGI
#define NOMINMAX
#include <d3d11.h>
#include <dxgi.h>
#undef NOMINMAX

namespace camy
{
	GPUBackend::GPUBackend() :
		m_device{ nullptr },
		m_context{ nullptr },
		m_factory{ nullptr },
		m_adapter{ nullptr },
		m_feature_level{ D3D_FEATURE_LEVEL_11_0 },

		m_postprocess_vs{ nullptr }
	{

	}

	GPUBackend::~GPUBackend()
	{
		close();
	}
	
	bool GPUBackend::open(u32 adapter_index_ignored)
	{
		HRESULT result{ S_OK };

		// Enumerating adapters and choosing the first one that meets the specs
		result = CreateDXGIFactory(camy_uuid_ptr(m_factory));
		if (FAILED(result)) 
		{ 
			camy_error("Failed to create DXGIFactory"); 
			return false; 
		}

		IDXGIAdapter* current_adapter{ nullptr };
		auto adapter_index{ 0u };
		while (m_factory->EnumAdapters(adapter_index, &current_adapter) != DXGI_ERROR_NOT_FOUND)
		{
			D3D_FEATURE_LEVEL feature_level = D3D_FEATURE_LEVEL_11_0;

			// Trying to create device with this adapter
			D3D_FEATURE_LEVEL fl;
			result = D3D11CreateDevice(current_adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_DEBUG,
				&feature_level, 1, D3D11_SDK_VERSION, &m_device, &fl, &m_context);

			if (SUCCEEDED(result))
			{
				m_adapter = current_adapter;
				m_adapter->AddRef();
				current_adapter->Release();
				break;
			}

			current_adapter->Release();
		}

		m_feature_level = (11 << 16);

		if (m_device == nullptr)
		{
			camy_error("Failed to find GPU with required feature level");
			close();
			return false;
		}

		DXGI_ADAPTER_DESC adapter_desc;
		m_adapter->GetDesc(&adapter_desc);
		
		char name_buffer[32];
		wcstombs(name_buffer, adapter_desc.Description, sizeof(name_buffer));
		camy_info("Adapter in use : ", name_buffer);
		
		for (auto i{ 0u }; i < Shader::num_types; ++i)
		{
			if (!m_cbuffers[i].load())
			{
				close();
				return false;
			}
		}

		if (!create_builtin_resources())
		{
			camy_error("Failed to create builtin resources for the GPUBackend, see previous error messages for more info");
			return false;
		}
			
		return true;
	}

	bool GPUBackend::create_builtin_resources()
	{
		m_postprocess_vs = create_shader(Shader::Type::Vertex, pp_vs, sizeof(pp_vs));
		if (m_postprocess_vs == nullptr)
			return false;

		return true;
	}

	void GPUBackend::close()
	{
		safe_release_com(m_context);
		safe_release_com(m_device);
		safe_release_com(m_adapter);
		safe_release_com(m_factory);
	}

	D3D11_PRIMITIVE_TOPOLOGY camy_to_d3d11(PrimitiveTopology primitive_topology)
	{
		switch (primitive_topology)
		{
		case PrimitiveTopology::PointList:
			return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
		case PrimitiveTopology::TriangleStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		case PrimitiveTopology::TriangleList:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case PrimitiveTopology::LineList:
			return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
		case PrimitiveTopology::LineStrip:
			return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
		default:
			return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		}
	}

	void GPUBackend::execute(const RenderLayer* render_layer)
	{
		// Warning should be issued by higher levels
		if (render_layer == nullptr)
			return;

		PipelineCache pc;

		for (auto rq{ 0u }; rq < render_layer->get_num_render_queues(); ++rq)
		{
			const auto& render_queue{ render_layer->get_render_queues()[rq] };

			// Getting queues
			const auto render_items{ render_queue.get_items() };
			const auto sorted_indices{ render_queue.get_sorted_indices() };
			const auto num_sorted_indices{ render_queue.get_num_sorted_indices() };

			// Warning should be issued by higher levels
			if (render_items == nullptr || sorted_indices == nullptr || num_sorted_indices == 0)
				return;

			for (auto i{ 0u }; i < num_sorted_indices; ++i)
			{
				auto& render_item{ render_items[sorted_indices[i]] };

				// Keeps track of states set this very iteration
				auto cur_states_set{ 0u };

				// Common states are cached altogether
				if (render_item.common_states == pc.common_states &&
					(pc.states_set & PipelineStates_CommonStates))
					cur_states_set |= PipelineStates_CommonStates;

				if (render_item.common_states != pc.common_states || 
					!(pc.states_set & PipelineStates_CommonStates))
				{
					set_common_states(*render_item.common_states);

					pc.common_states = render_item.common_states;
					pc.states_set |= PipelineStates_CommonStates;

					cur_states_set |= PipelineStates_CommonStates;
				}

				// Vertex buffer slot 0
				if (render_item.vertex_buffer1 != pc.vertex_buffer1 ||
					!(pc.states_set & PipelineStates_VertexBuffer1))
				{
					ID3D11Buffer* vb{ nullptr };
					u32			  stride{ 0 };
					u32			  offset{ 0 };

					if (render_item.vertex_buffer1 != nullptr)
					{
						vb = render_item.vertex_buffer1->hidden.buffer;
						stride = render_item.vertex_buffer1->element_size;

						camy_validate_state(vb, "Binding null vertex buffer at slot 0");
					}

					m_context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
					pc.vertex_buffer1 = render_item.vertex_buffer1;
					pc.states_set |= PipelineStates_VertexBuffer1;

					cur_states_set |= PipelineStates_VertexBuffer1;
				}
				
				// Vertex buffer slot 1
				if (render_item.vertex_buffer2 != pc.vertex_buffer2 ||
					!(pc.states_set & PipelineStates_VertexBuffer2))
				{
					ID3D11Buffer* vb{ nullptr };
					u32			  stride{ 0 };
					u32			  offset{ 0 };

					if (render_item.vertex_buffer2 != nullptr)
					{
						vb = render_item.vertex_buffer2->hidden.buffer;
						stride = render_item.vertex_buffer2->element_size;

						camy_validate_state(vb, "Binding null vertex buffer at slot 1");
					}

					m_context->IASetVertexBuffers(1, 1, &vb, &stride, &offset);
					pc.vertex_buffer2 = render_item.vertex_buffer2;
					pc.states_set |= PipelineStates_VertexBuffer2;

					cur_states_set |= PipelineStates_VertexBuffer2;
				}

				// Index buffer
				if (render_item.index_buffer != pc.index_buffer ||
					!(pc.states_set & PipelineStates_IndexBuffer))
				{
					ID3D11Buffer* ib{ nullptr };
					DXGI_FORMAT format{ DXGI_FORMAT_R16_UINT };

					if (render_item.index_buffer != nullptr)
					{
						ib = render_item.index_buffer->hidden.buffer;

						if (render_item.index_buffer->index_type == IndexBuffer::Type::U32)
							format = DXGI_FORMAT_R32_UINT;
	
						camy_validate_state(ib, "Binding null index buffer");
					}

					m_context->IASetIndexBuffer(ib, format, 0);

					pc.index_buffer = render_item.index_buffer;
					pc.states_set |= PipelineStates_IndexBuffer;

					cur_states_set |= PipelineStates_IndexBuffer;
				}

				// Primitive topology
				if (render_item.draw_info.primitive_topology != pc.primitive_topology ||
					!(pc.states_set & PipelineStates_PrimitiveTopology))
				{
					m_context->IASetPrimitiveTopology(camy_to_d3d11(render_item.draw_info.primitive_topology));
					
					pc.primitive_topology = render_item.draw_info.primitive_topology;
					pc.states_set |= PipelineStates_PrimitiveTopology;

					cur_states_set |= PipelineStates_PrimitiveTopology;
				}

				// Setting shader
				if (render_item.vertex_shader != pc.shaders[static_cast<u32>(Shader::Type::Vertex)]||
					!(pc.states_set & PipelineStates_VertexShader))
				{
					ID3D11VertexShader* vertex_shader{ nullptr };
					ID3D11InputLayout* input_layout{ nullptr };

					if (render_item.vertex_shader != nullptr)
					{
						vertex_shader = static_cast<ID3D11VertexShader*>(render_item.vertex_shader->m_shader->shader);
						input_layout = render_item.vertex_shader->m_input_signature->hidden.input_layout;
					}
					// Binding shader itself
					m_context->VSSetShader(vertex_shader, nullptr, 0);
					
					// Input layout since they are bound togheter, could cache it apart but probably not worth in the end
					m_context->IASetInputLayout(input_layout);

					pc.shaders[static_cast<u32>(Shader::Type::Vertex)] = render_item.vertex_shader;
					pc.states_set |= PipelineStates_VertexShader;

					cur_states_set |= PipelineStates_VertexShader;
				}

				if (render_item.pixel_shader != pc.shaders[static_cast<u32>(Shader::Type::Pixel)] ||
					!(pc.states_set & PipelineStates_PixelShader))
				{
					ID3D11PixelShader* pixel_shader{ nullptr };
					
					if (render_item.pixel_shader != nullptr)
						pixel_shader = static_cast<ID3D11PixelShader*>(render_item.pixel_shader->m_shader->shader);

					m_context->PSSetShader(pixel_shader, nullptr, 0);

					pc.shaders[static_cast<u32>(Shader::Type::Pixel)] = render_item.pixel_shader;
					pc.states_set |= PipelineStates_VertexShader;
				}

				// Setting shared parameters
				// Exlusivity between shared_parameters and single parameters has to be guaranteed
				// by the user
				if (i == 0)
				{
					auto shared_parameters{ render_queue.get_shared_parameters() };
					set_parameters(shared_parameters, pc);
				}

				// Time to set parameters
				for (auto pg{ 0u }; pg < render_item.num_cached_parameter_groups; ++pg)
				{
					auto& cached_parameter_group{ render_item.cached_parameter_groups[pg] };

					// No, he wasn't 
					auto& parameter_group{ cached_parameter_group.parameter_group };

					if (parameter_group == nullptr)
					{
						camy_warning("Skipping cached parameter gruop slot: ", cached_parameter_group.cache_slot, " & index: ", pg, ". Associated ParameterGroup is null");
						continue;
					}
					
					// First off we check if the previous render item was using the very same parametergroup in the very samy
					// cache slot
					if (pc.parameter_cache[cached_parameter_group.cache_slot] == nullptr ||
						pc.parameter_cache[cached_parameter_group.cache_slot] != cached_parameter_group.parameter_group)
					{
						// Setting this parameter group 
						// For additional caching ( such as cbuffers ) see set_parameters
						set_parameters(*parameter_group, pc);

						// Updating cache slot with this very parameter group
						pc.parameter_cache[cached_parameter_group.cache_slot] = cached_parameter_group.parameter_group;
					}
				}

				// Before issuing the draw command, we need to set to default all the other states,
				// part of it is already done since we are checking whether this state is different from the previous one, 
				// and if it is ( even null ) it is set as so
				if (!(cur_states_set & PipelineStates_CommonStates))
					set_default_common_states();

				m_context->DrawIndexed(render_item.draw_info.index_count, render_item.draw_info.index_offset, render_item.draw_info.vertex_offset);
			}

			// It's now time to check dependencies
			// We could simply reset ever single state of every single shader, but usually dependencies are very limited
			for (auto i{ 0u }; i < render_queue.get_num_dependencies(); ++i)
				unbind_dependency(render_queue.get_dependencies()[i]);
		}
	}

	void GPUBackend::execute(const ComputeItem& compute_item)
	{
		if (!(compute_item.group_countx + compute_item.group_county + compute_item.group_countz))
		{
			camy_warning("Skipping compute item, total dispatch count is 0");
			return;
		}

		// Setting shader
		if (compute_item.compute_shader == nullptr ||
			compute_item.compute_shader->m_shader == nullptr)
		{
			camy_warning("Skipping compute item, no program specified");
			return;
		}
		m_context->CSSetShader(static_cast<ID3D11ComputeShader*>(compute_item.compute_shader->m_shader->shader), nullptr, 0);

		// Setting parameters
		PipelineCache pc; // Not needed, here to avoid writing multiple methods 
		set_parameters(compute_item.parameters, pc);

		// Dispatching
		m_context->Dispatch(compute_item.group_countx, compute_item.group_county, compute_item.group_countz);
	
		// I am currently unbinding everythin
		for (auto i{ 0u }; i < compute_item.parameters.num_parameters; ++i)
			unbind_dependency(compute_item.parameters.parameters[i].shader_variable);
	}

	void GPUBackend::execute(const ComputeLayer* compute_layer)
	{
		// Currently there is no caching in the compute pipeline
		const auto& compute_queue{ *compute_layer->get_queue() };

		const auto compute_items{ compute_queue.get_items() };
		const auto sorted_indices{ compute_queue.get_sorted_indices() };
		const auto num_sorted_indices{ compute_queue.get_num_sorted_indices() };

		// Warning should be issued by higher levels
		if (compute_items == nullptr || sorted_indices == nullptr || num_sorted_indices == 0)
			return;

		for (auto i{ 0u }; i < num_sorted_indices; ++i)
		{
			const auto& compute_item{ compute_items[sorted_indices[i]] };

			execute(compute_item);
		}
	}

	void GPUBackend::execute(const PostProcessLayer* pp_layer)
	{
		if (pp_layer == nullptr)
		{
			camy_warning("Calling execute on a null layer");
			return;
		}

		// Applying shared states
		PipelineCache pc;
		auto parameters{ pp_layer->get_shared_parameters() };
		if (parameters != nullptr)
			set_parameters(*parameters, pc);
		

		// Binding shared vertex shader
		m_context->VSSetShader(static_cast<ID3D11VertexShader*>(m_postprocess_vs->shader), nullptr, 0);

		ID3D11ShaderResourceView* previous_srv{ nullptr };
		for (u32 i{ 0u }; i < pp_layer->get_num_items(); ++i)
		{
			const auto& pp_item{ pp_layer->get_items()[i] };

			if (pp_item.pixel_shader == nullptr || 
				pp_item.output_surface == nullptr)
			{
				camy_warning("Can't execute post processing item without a pixel shader or an output surface specified specified");
				continue;
			}

			m_context->PSSetShader(static_cast<ID3D11PixelShader*>(pp_item.pixel_shader->m_shader->shader), nullptr, 0);

			// Binding previous render target as SRV resource one
			ID3D11ShaderResourceView* srv_slot0{ previous_srv };
			if (pp_item.input_surface != nullptr)
				srv_slot0 = pp_item.input_surface->hidden.srv;
	
			set_parameters(pp_item.parameters, pc);

			m_context->OMSetRenderTargets(1, &pp_item.output_surface->hidden.rtv, nullptr);

			// Binding 
			m_context->PSSetShaderResources(0, 1, &srv_slot0);

			D3D11_VIEWPORT viewport;
			viewport.TopLeftX =
				viewport.TopLeftY = 0.f;
			viewport.Width = static_cast<float>(pp_item.output_surface->description.width);
			viewport.Height = static_cast<float>(pp_item.output_surface->description.height);
			viewport.MinDepth = 0.f;
			viewport.MaxDepth = 1.f;

			m_context->RSSetViewports(1, &viewport);

			// Blend state, no caching atm nor nullptr check ( Todo ) 
			if (pp_item.blend_state != nullptr)
				m_context->OMSetBlendState(pp_item.blend_state->hidden.state, nullptr, 0xFFFFFFFF);
			else
				m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);

			// Issuing draw call
			m_context->Draw(3, 0);

			// Preparing for next iteration
			ID3D11ShaderResourceView* null_srv{ nullptr };
			m_context->PSSetShaderResources(0, 1, &null_srv);
			previous_srv = pp_item.output_surface->hidden.srv;

			// Unbinding last render target
			ID3D11RenderTargetView* null_rtv{ nullptr };
			m_context->OMSetRenderTargets(1, &null_rtv, nullptr);

			// Todo: implement dependencies on postprocess items aswell, currently unbinding, 
			// this is bad but first want to make sure the rest is working
			for (auto p{ 0u }; p < pp_item.parameters.num_parameters; ++p)
				unbind_dependency(pp_item.parameters.parameters[p].shader_variable);
		}
	}

	void GPUBackend::update(const Buffer* buffer, const void* data)
	{
		if (buffer == nullptr ||
			buffer->hidden.buffer == nullptr)
		{
			camy_error("Can't update null buffer / resource");
			return;
		}

		if (data == nullptr)
		{
			camy_error("Can't update buffer with null data");
			return;
		}

		if (buffer->is_dynamic)
		{
			D3D11_MAPPED_SUBRESOURCE mapped_buffer;
			m_context->Map(buffer->hidden.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_buffer);
			std::memcpy(mapped_buffer.pData, data, buffer->element_count * buffer->element_size);
			m_context->Unmap(buffer->hidden.buffer, 0);
		}
		else
		{
			D3D11_BOX box;
			ZeroMemory(&box, sizeof(D3D11_BOX));
			box.right = buffer->element_size * buffer->element_count;
			box.back = 1;
			box.bottom = 1;
			m_context->UpdateSubresource(buffer->hidden.buffer, 0, &box, data, 0, 0);
		}
	}

	InputSignature* GPUBackend::create_input_signature(const void* compiled_bytecode, Size bytecode_size, const void* inputs, Size num_inputs)
	{
		camy_assert(inputs != nullptr, { return; }, "Failed to create input signature, inputs is null");
		const auto d3d11_inputs{ static_cast<const D3D11_INPUT_ELEMENT_DESC*>(inputs) };
		
		ID3D11InputLayout* input_layout;
		auto result{ m_device->CreateInputLayout(d3d11_inputs, static_cast<UINT>(num_inputs), compiled_bytecode, bytecode_size, &input_layout) };
		if (FAILED(result))
		{
			camy_error("Failed to create input signature");
			return nullptr;
		}

		auto input_signature{ m_resources.allocate<InputSignature>() };
		input_signature->hidden.input_layout = input_layout;

		return input_signature;
	}

	Buffer* GPUBackend::create_buffer(Buffer::Type type, u32 num_elements, u32 element_size, bool use_uav)
	{
		bool is_dynamic{ true };
		D3D11_BUFFER_DESC cb_desc;
		cb_desc.ByteWidth = num_elements * element_size;
		cb_desc.Usage = D3D11_USAGE_DYNAMIC;
		cb_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		if (use_uav)
		{
			cb_desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			cb_desc.Usage = D3D11_USAGE_DEFAULT;
			is_dynamic = false;
			camy_info("Unordered access resources cannot be dynamic, using default usage");
		}
		cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cb_desc.StructureByteStride = element_size;

		UINT misc_flag;
		switch (type)
		{
		case Buffer::Type::Structured:
			misc_flag = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; break;
		default:
			misc_flag = 0;
		}
		cb_desc.MiscFlags = misc_flag;

		ID3D11Buffer* buffer{ nullptr };
		auto result{ m_device->CreateBuffer(&cb_desc, nullptr, &buffer) };
		if (FAILED(result))
		{
			camy_error("Failed to create buffer | ", num_elements, " | ", element_size);
			return nullptr;
		}

		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
		srv_desc.Format = DXGI_FORMAT_UNKNOWN; 
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srv_desc.Buffer.ElementOffset = 0;
		srv_desc.Buffer.NumElements = num_elements;

		ID3D11ShaderResourceView* srv{ nullptr };
		result = m_device->CreateShaderResourceView(buffer, &srv_desc, &srv);
		
		if (FAILED(result))
		{
			camy_error("Failed to create buffer srv | ", element_size, " | ", num_elements);
			safe_release_com(buffer);
			return false;
		}

		ID3D11UnorderedAccessView* uav{ nullptr };
		if (use_uav)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
			uav_desc.Format = DXGI_FORMAT_UNKNOWN;
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
			uav_desc.Buffer.FirstElement = 0;
			uav_desc.Buffer.NumElements = num_elements;
			uav_desc.Buffer.Flags = 0; // Todo: Here we should switch based on type, currently only RWStructuredBuffer w/o counter is used/supported
		
			result = m_device->CreateUnorderedAccessView(buffer, &uav_desc, &uav);
			if (FAILED(result))
			{
				safe_release_com(srv);
				safe_release_com(buffer);
				return false;
			}
		}


		auto buffer_r{ m_resources.allocate<Buffer>() };
		buffer_r->element_count = num_elements;
		buffer_r->element_size = element_size;
		buffer_r->type = type;
		buffer_r->hidden.buffer = buffer;
		buffer_r->hidden.srv = srv;
		buffer_r->hidden.uav = uav;
		buffer_r->is_dynamic = is_dynamic;

		return buffer_r;
	}

	VertexBuffer* GPUBackend::create_vertex_buffer(u32 element_size, u32 num_elements, const void* data, bool is_dynamic)
	{
		D3D11_BUFFER_DESC vb_desc;
		vb_desc.ByteWidth = num_elements * element_size;
		vb_desc.Usage = is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vb_desc.CPUAccessFlags = is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		vb_desc.MiscFlags = 0;
		vb_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vb_data{ 0 };
		vb_data.pSysMem = data;
		vb_data.SysMemPitch = vb_data.SysMemSlicePitch = 0;

		ID3D11Buffer* buffer{ nullptr };
		auto result{ m_device->CreateBuffer(&vb_desc, data != nullptr ? &vb_data : nullptr, &buffer) };
		if (FAILED(result))
		{
			camy_error("Failed to create vertex buffer | ", element_size, " | ", num_elements); 
			return nullptr;
		}

		auto vertex_buffer_r{ m_resources.allocate<VertexBuffer>() };
		vertex_buffer_r->element_size = element_size;
		vertex_buffer_r->element_count = num_elements;
		vertex_buffer_r->is_dynamic = is_dynamic;

		vertex_buffer_r->hidden.buffer = buffer;

		return vertex_buffer_r;
	}

	IndexBuffer* GPUBackend::create_index_buffer(IndexBuffer::Type index_type, u32 num_elements, const void* data, bool is_dynamic)
	{
		auto index_size{ index_type == IndexBuffer::Type::U16 ? 2 : 4 };

		D3D11_BUFFER_DESC ib_desc;
		ib_desc.ByteWidth = index_size * num_elements;
		ib_desc.Usage = is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		ib_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		ib_desc.CPUAccessFlags = is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		ib_desc.MiscFlags = 0;
		ib_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA ib_data{ 0 };
		ib_data.pSysMem = data;
		ib_data.SysMemPitch = ib_data.SysMemSlicePitch = 0;

		ID3D11Buffer* buffer{ nullptr };
		auto result{ m_device->CreateBuffer(&ib_desc, data != nullptr ? &ib_data : nullptr, &buffer) };
		if (FAILED(result))
		{
			camy_error("Failed to create index buffer | ", num_elements);
			return nullptr;
		}

		auto index_buffer_r{ m_resources.allocate<IndexBuffer>() };
		index_buffer_r->index_type = index_type;
		index_buffer_r->element_count = num_elements;

		index_buffer_r->hidden.buffer = buffer;

		return index_buffer_r;
	}

	ConstantBuffer* GPUBackend::create_constant_buffer(u32 size)
	{
		assert(size % 16 == 0);

		D3D11_BUFFER_DESC cb_desc;
		cb_desc.ByteWidth = size;
		cb_desc.Usage = D3D11_USAGE_DYNAMIC;
		cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		cb_desc.MiscFlags = 0;
		cb_desc.StructureByteStride = 0;

		ID3D11Buffer* buffer{ nullptr };
		auto result{ m_device->CreateBuffer(&cb_desc, nullptr, &buffer) };
		if (FAILED(result))
		{
			camy_error("Failed to create constant buffer | ", size);
			return nullptr;
		}

		auto cbuffer_r{ m_resources.allocate<ConstantBuffer>() };
		cbuffer_r->size = size;
		cbuffer_r->hidden.buffer = buffer;

		return cbuffer_r;
	}

	DXGI_FORMAT camy_to_dxgi(Surface::Format format)
	{
		switch (format)
		{
		case Surface::Format::Unknown:
			return DXGI_FORMAT_UNKNOWN;

			// Compressed formats
		case Surface::Format::BC1Unorm:
			return DXGI_FORMAT_BC1_UNORM;
		case Surface::Format::BC3Unorm:
			return DXGI_FORMAT_BC3_UNORM;

			// Typeless formats
		case Surface::Format::R16Typeless:
			return DXGI_FORMAT_R16_TYPELESS;
		case Surface::Format::R32Typeless:
			return DXGI_FORMAT_R32_TYPELESS;
		case Surface::Format::R24G8Typeless:
			return DXGI_FORMAT_R24G8_TYPELESS;
		case Surface::Format::R24UnormX8Typeless:
			return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

			// Uncompressed formats
		case Surface::Format::RGBA8Unorm:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case Surface::Format::RGBA16Float:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		case Surface::Format::RGBA32Float:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case Surface::Format::R16Float:
			return DXGI_FORMAT_R16_FLOAT;
		case Surface::Format::R16Unorm:
			return DXGI_FORMAT_R16_UNORM;
		case Surface::Format::R32Float:
			return DXGI_FORMAT_R32_FLOAT;

			// Depth formats
		case Surface::Format::D16Unorm:
			return DXGI_FORMAT_D16_UNORM;
		case Surface::Format::D32Float:
			return DXGI_FORMAT_D32_FLOAT;
		case Surface::Format::D24UNorm_S8Uint:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;

		default:
			camy_warning("Failed to translate format to backend API, not supported: ", static_cast<u32>(format));
			return DXGI_FORMAT_UNKNOWN;
		}
	}

	Surface* GPUBackend::create_texture2D(Surface::Format format, u32 width, u32 height, const SubSurface* subsurfaces, u32 num_subsurfaces, bool is_dynamic, u8 msaa_level)
	{
		Surface::Description description;
		description.format = format;
		description.format_srv = format;
		description.width = width;
		description.height = height;
		description.msaa_level = msaa_level;
		description.is_dynamic = is_dynamic;

		return create_surface(description, true, false, false, false, subsurfaces, num_subsurfaces);
	}

	Surface* GPUBackend::create_render_target(Surface::Format format, u32 width, u32 height, u8 msaa_level)
	{
		Surface::Description description;
		description.format = format;
		description.format_srv = format;
		description.format_rtv = format;
		description.width = width;
		description.height = height;
		description.msaa_level = msaa_level;
	
		return create_surface(description, true, true, false, nullptr, false);
	}

	Surface* GPUBackend::create_depth_buffer(Surface::Format format, u32 width, u32 height, u8 msaa_level)
	{
		Surface::Description description;
		description.format = format;
		description.format_dsv = format;
		description.width = width;
		description.height = height;
		description.msaa_level = msaa_level;

		return create_surface(description, false, false, true, nullptr, false);
	}

	Surface* GPUBackend::create_surface(const Surface::Description& description, bool use_srv, bool use_rtv, bool use_dsv, bool use_uav, const SubSurface* subsurfaces, u32 num_subsurfaces)
	{
		camy_assert(description.width > 0 && description.height > 0, { return nullptr; }, "Trying to create texture with width or height == 0");
		camy_assert(description.msaa_level > 0, { return nullptr; }, "MSAA Level has to be > 0 ");

		// Creating texture2D
		auto format{ camy_to_dxgi(description.format) };

		D3D11_TEXTURE2D_DESC t2_desc;
		t2_desc.Width = description.width;
		t2_desc.Height = description.height;

		// Cubemaps or arrays not supported yet ( TODO ) 
		if (subsurfaces != nullptr)
			t2_desc.MipLevels = num_subsurfaces;
		else
			t2_desc.MipLevels = 1;
		t2_desc.ArraySize = 1;
		t2_desc.Format = format;
		if (description.msaa_level > 1)
		{
			UINT quality;
			m_device->CheckMultisampleQualityLevels(t2_desc.Format, description.msaa_level, &quality);
			if (quality == 0)
			{
				camy_error("Failed to create surface with multisample level : ", std::to_string(description.msaa_level).c_str());
				return false;
			}

			t2_desc.SampleDesc.Count = description.msaa_level;
			t2_desc.SampleDesc.Quality = quality - 1;
		}
		else
		{
			t2_desc.SampleDesc.Count = 1;
			t2_desc.SampleDesc.Quality = 0;
		}
		t2_desc.Usage = description.is_dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		t2_desc.BindFlags = 0;
		if (use_srv) t2_desc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
		if (use_rtv) t2_desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
		if (use_dsv) t2_desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
		if (use_uav) t2_desc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		t2_desc.CPUAccessFlags = description.is_dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		t2_desc.MiscFlags = 0;

		std::vector<D3D11_SUBRESOURCE_DATA> ssd;
		for (auto i{ 0u }; i < num_subsurfaces; ++i)
		{
			ssd.emplace_back();
			ssd.back().pSysMem = subsurfaces[i].data;
			ssd.back().SysMemPitch = subsurfaces[i].pitch;
		}

		ID3D11Texture2D* texture{ nullptr };
		auto result{ m_device->CreateTexture2D(&t2_desc, subsurfaces == nullptr ? nullptr : &ssd[0], &texture) };
		if (FAILED(result))
		{
			camy_error("Failed to create Texture2D ");
			return nullptr;
		}

		ID3D11ShaderResourceView* srv{ nullptr };
		ID3D11RenderTargetView* rtv{ nullptr };
		ID3D11DepthStencilView* dsv{ nullptr };
		ID3D11UnorderedAccessView* uav{ nullptr };

		if (use_srv)
		{
			auto format_srv{ camy_to_dxgi(description.format_srv) };

			D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
			srv_desc.Format = format_srv;
			srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			srv_desc.Texture2D.MipLevels = t2_desc.MipLevels;
			srv_desc.Texture2D.MostDetailedMip = 0;
			result = m_device->CreateShaderResourceView(texture, &srv_desc, &srv);
			if (FAILED(result))
			{
				camy_error("Failed to create shader resource view for surface");
				safe_release_com(texture);
				return nullptr;
			}
		}

		if (use_rtv)
		{
			auto format_rtv{ camy_to_dxgi(description.format_rtv) };

			D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
			rtv_desc.Format = format_rtv;
			rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
			rtv_desc.Texture2D.MipSlice = 0;
			result = m_device->CreateRenderTargetView(texture, &rtv_desc, &rtv);
			if (FAILED(result))
			{
				camy_error("Failed to create render target view for surface");
				safe_release_com(srv);
				safe_release_com(texture);
				return nullptr;
			}
		}

		if (use_dsv)
		{
			auto format_dsv{ camy_to_dxgi(description.format_dsv) };
		
			D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc;
			dsv_desc.Flags = 0;
			dsv_desc.Format = format_dsv;
			dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			dsv_desc.Texture2D.MipSlice = 0;
			result = m_device->CreateDepthStencilView(texture, &dsv_desc, &dsv);
			if (FAILED(result))
			{
				camy_error("Failed to create depth stencil view for surface");
				safe_release_com(rtv);
				safe_release_com(srv);
				safe_release_com(texture);
				return nullptr;
			}
		}

		if (use_uav)
		{
			auto format_uav{ camy_to_dxgi(description.format_uav) };

			D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
			uav_desc.Format = format_uav;
			uav_desc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
			uav_desc.Texture2D.MipSlice = 0;
			result = m_device->CreateUnorderedAccessView(texture, &uav_desc, &uav);
			if (FAILED(result))
			{
				camy_error("Failed to create unordered access view for surface");
				safe_release(dsv);
				safe_release(rtv);
				safe_release(srv);
				safe_release(texture);
				return nullptr;
			}
		}

		auto surface_r{ m_resources.allocate<Surface>() };
		surface_r->description = description;
		surface_r->hidden.texture_2d = texture;
		surface_r->hidden.srv = srv;
		surface_r->hidden.rtv = rtv;
		surface_r->hidden.dsv = dsv;
		surface_r->hidden.uav = uav;

		return surface_r;
	}

	void compile_from_camy(BlendState::Mode blend_mode, D3D11_BLEND_DESC& bs_desc)
	{
		bs_desc.AlphaToCoverageEnable = false; // NO MSAA support yet
		bs_desc.IndependentBlendEnable = false; // Not supported yet
		bs_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;  
		
		switch (blend_mode)
		{
		case BlendState::Mode::Opaque:
			bs_desc.RenderTarget[0].BlendEnable = true;
			bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
			bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			break;

		case BlendState::Mode::Transparent:
			bs_desc.RenderTarget[0].BlendEnable = true;
			bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
			bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO ;
			break;

		case BlendState::Mode::Additive:
			bs_desc.RenderTarget[0].BlendEnable = true;
			bs_desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			bs_desc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
			bs_desc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
			bs_desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
			bs_desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			break;
		}
	}

	BlendState* GPUBackend::create_blend_state(BlendState::Mode blend_mode)
	{
		D3D11_BLEND_DESC bs_desc;
		compile_from_camy(blend_mode, bs_desc);

		ID3D11BlendState* blend_state{ nullptr };
		auto result { m_device->CreateBlendState(&bs_desc, &blend_state) };
		if (FAILED(result))
		{
			camy_error("Failed to create blend state");
			return nullptr;
		}
			
		auto blend_state_r{ m_resources.allocate<BlendState>() };
		blend_state_r->mode = blend_mode;
		blend_state_r->hidden.state = blend_state;

		return blend_state_r;
	}

	D3D11_FILL_MODE camy_to_d3d11(RasterizerState::Fill fill_mode)
	{
		switch (fill_mode)
		{
		case RasterizerState::Fill::Solid:
			return D3D11_FILL_SOLID;
		case RasterizerState::Fill::Wireframe:
			return D3D11_FILL_WIREFRAME;
		default:
			return D3D11_FILL_SOLID;
		}
	}

	D3D11_CULL_MODE camy_to_d3d11(RasterizerState::Cull cull_mode)
	{
		switch (cull_mode)
		{
		case RasterizerState::Cull::Back:
			return D3D11_CULL_BACK;
		case RasterizerState::Cull::Front:
			return D3D11_CULL_FRONT;
		case RasterizerState::Cull::None:
			return D3D11_CULL_NONE;
		default:
			return D3D11_CULL_NONE; // Default d3d11 is back but None IMHO Makes the user notice that there is something wrong :)
		}
	}

	RasterizerState* GPUBackend::create_rasterizer_state(RasterizerState::Cull cull, RasterizerState::Fill fill, u32 bias,  float bias_max, float bias_slope)
	{
		D3D11_RASTERIZER_DESC rs_desc;
		rs_desc.CullMode = camy_to_d3d11(cull);
		rs_desc.FillMode = camy_to_d3d11(fill);
		rs_desc.FrontCounterClockwise = false;
		rs_desc.DepthBias = bias;
		rs_desc.DepthBiasClamp = bias_max;
		rs_desc.SlopeScaledDepthBias = bias_slope;
		rs_desc.DepthClipEnable = true;
		rs_desc.ScissorEnable = false;
		rs_desc.MultisampleEnable = true;
		rs_desc.AntialiasedLineEnable = false;
		
		ID3D11RasterizerState* rasterizer_state;
		auto result{ m_device->CreateRasterizerState(&rs_desc, &rasterizer_state) };
		if (FAILED(result))
		{ 
			camy_error("Failed to create rasterizer state");
			return nullptr;
		}

		auto rasterizer_state_r{ m_resources.allocate<RasterizerState>() };
		rasterizer_state_r->cull = cull;
		rasterizer_state_r->fill = fill;
		rasterizer_state_r->hidden.state = rasterizer_state;

		return rasterizer_state_r;
	}

	D3D11_TEXTURE_ADDRESS_MODE camy_to_d3d11(Sampler::Address address_mode)
	{
		switch (address_mode)
		{
		case Sampler::Address::Clamp:
			return D3D11_TEXTURE_ADDRESS_CLAMP;
		case Sampler::Address::Wrap:
			return D3D11_TEXTURE_ADDRESS_WRAP;
		case Sampler::Address::Mirror:
			return D3D11_TEXTURE_ADDRESS_MIRROR;
		default:
			return D3D11_TEXTURE_ADDRESS_CLAMP;
		}
	}

	D3D11_FILTER camy_to_d3d11(Sampler::Filter filter, bool comparison)
	{
		switch (filter)
		{
		case Sampler::Filter::Point:
			return comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT : D3D11_FILTER_MIN_MAG_MIP_POINT;
		case Sampler::Filter::Linear:
			return comparison ? D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case Sampler::Filter::Anisotropic:
			return comparison ? D3D11_FILTER_COMPARISON_ANISOTROPIC : D3D11_FILTER_ANISOTROPIC;
		default:
			return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		}
	}

	D3D11_COMPARISON_FUNC camy_to_d3d11(Sampler::Comparison comparison)
	{
		switch (comparison)
		{
		case Sampler::Comparison::Less:
			return D3D11_COMPARISON_LESS;
		case Sampler::Comparison::LessEqual:
			return D3D11_COMPARISON_LESS_EQUAL;
		default:
			return D3D11_COMPARISON_NEVER;
		}
	}

	Sampler* GPUBackend::create_sampler(Sampler::Filter filter, Sampler::Address address, Sampler::Comparison comparison)
	{
		D3D11_SAMPLER_DESC s_desc;
		s_desc.Filter = camy_to_d3d11(filter, comparison != Sampler::Comparison::Never);
		s_desc.AddressU =
			s_desc.AddressV =
			s_desc.AddressW = camy_to_d3d11(address);
		s_desc.MinLOD = -FLT_MAX;
		s_desc.MaxLOD = FLT_MAX;
		s_desc.MipLODBias = 0.f;
		s_desc.MaxAnisotropy = filter == Sampler::Filter::Anisotropic ? D3D11_MAX_MAXANISOTROPY : 0;
		s_desc.ComparisonFunc = camy_to_d3d11(comparison);
		s_desc.BorderColor[0] =
			s_desc.BorderColor[1] =
			s_desc.BorderColor[2] =
			s_desc.BorderColor[3] = 1.f;
		
		ID3D11SamplerState* sampler;
		auto result{ m_device->CreateSamplerState(&s_desc, &sampler) };
		if (FAILED(result))
		{
			camy_error("Failed to create sampler state");
			return nullptr;
		}

		auto sampler_r{ m_resources.allocate<Sampler>() };
		sampler_r->address = address;
		sampler_r->filter = filter;
		sampler_r->comparison = comparison;

		sampler_r->hidden.sampler = sampler;

		return sampler_r;
	}

	hidden::Shader* GPUBackend::create_shader(Shader::Type type, const void* compiled_bytecode, Size bytecode_size)
	{
		assert(compiled_bytecode != nullptr);
		assert(bytecode_size > 0);

		auto result{ S_OK };

		ID3D11DeviceChild* shader{ nullptr };
		switch (type)
		{
		case Shader::Type::Vertex:
			result = m_device->CreateVertexShader(compiled_bytecode, bytecode_size, nullptr, reinterpret_cast<ID3D11VertexShader**>(&shader));
			break;
		case Shader::Type::Pixel:
			result = m_device->CreatePixelShader(compiled_bytecode, bytecode_size, nullptr, reinterpret_cast<ID3D11PixelShader**>(&shader));
			break;
		case Shader::Type::Geometry:
			result = m_device->CreateGeometryShader(compiled_bytecode, bytecode_size, nullptr, reinterpret_cast<ID3D11GeometryShader**>(&shader));
			break;
		case Shader::Type::Compute:
			result = m_device->CreateComputeShader(compiled_bytecode, bytecode_size, nullptr, reinterpret_cast<ID3D11ComputeShader**>(&shader));
			break;
		default:
			{
				camy_error("Not supported shader type");
				return false;
			}
		}

		if (FAILED(result))
		{
			camy_error("Failed to create shader");
			return false;
		}

		auto shader_r{ m_resources.allocate<hidden::Shader>() };
		shader_r->shader = shader;

		return shader_r;
	}

	DepthStencilState* GPUBackend::create_depth_stencil_state()
	{
		D3D11_DEPTH_STENCIL_DESC dss_desc{ 0 };
		dss_desc.DepthEnable = true;
		dss_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		dss_desc.DepthFunc = D3D11_COMPARISON_LESS;
		dss_desc.StencilEnable = false;

		ID3D11DepthStencilState* state;
		auto result{ m_device->CreateDepthStencilState(&dss_desc, &state) };

		auto dss_r{ m_resources.allocate<DepthStencilState>() };
		dss_r->hidden.state = state;


		return dss_r;
	}

	Surface* GPUBackend::create_window_surface(WindowHandle window_handle,u8 msaa_level)
	{
		assert(msaa_level > 0);

		RECT window_rect;
		GetWindowRect(static_cast<HWND>(window_handle), &window_rect);

		DXGI_SWAP_CHAIN_DESC sc_desc;
		sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		sc_desc.BufferDesc.Width = window_rect.right - window_rect.left;
		sc_desc.BufferDesc.Height = window_rect.bottom - window_rect.top;
		sc_desc.BufferDesc.RefreshRate.Numerator = 0;
		sc_desc.BufferDesc.RefreshRate.Denominator = 1;
		sc_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		sc_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

		if (msaa_level > 1)
		{
			UINT quality;
			m_device->CheckMultisampleQualityLevels(sc_desc.BufferDesc.Format, msaa_level, &quality);
			if (quality == 0)
			{
				camy_error("Failed to create window target with multisample level : ", std::to_string(msaa_level).c_str());
				return false;
			}

			sc_desc.SampleDesc.Count = msaa_level;
			sc_desc.SampleDesc.Quality = quality - 1;
		}
		else
		{
			sc_desc.SampleDesc.Count = 1;
			sc_desc.SampleDesc.Quality = 0;
		}

		sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		sc_desc.BufferCount = 1;
		sc_desc.OutputWindow = static_cast<HWND>(window_handle);
		sc_desc.Windowed = true;
		sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sc_desc.Flags = 0;

		IDXGISwapChain* swap_chain;
		auto result { m_factory->CreateSwapChain(m_device, &sc_desc, &swap_chain) };
		if (FAILED(result))
		{
			camy_error("Failed to create swap chain");
			return nullptr;
		}

		ID3D11Texture2D* bb_texture{ nullptr };
		result = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&bb_texture));
		if (FAILED(result))
		{
			camy_error("Failed to retrieve backbuffer");
			safe_release_com(swap_chain);
			return nullptr;
		}

		ID3D11RenderTargetView* rtv{ nullptr };
		result = m_device->CreateRenderTargetView(bb_texture, nullptr, &rtv);
		if (FAILED(result))
		{
			camy_error("Failed to create render target view for window surface");
			safe_release_com(bb_texture);
			safe_release_com(swap_chain);
			return nullptr;
		}

		D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
		rtv->GetDesc(&rtv_desc);

		ID3D11ShaderResourceView* srv;
		result = m_device->CreateShaderResourceView(bb_texture, nullptr, &srv);
		if (FAILED(result))
		{
			camy_error("Failed to create shader resource view for window surface");
			safe_release_com(bb_texture);
			safe_release_com(swap_chain);
			safe_release_com(rtv);
			return nullptr;
		}

		auto window_surface_r{ m_resources.allocate<Surface>() };
		window_surface_r->description.width = sc_desc.BufferDesc.Width;
		window_surface_r->description.height = sc_desc.BufferDesc.Height;
		window_surface_r->description.format = Surface::Format::RGBA8Unorm;
		window_surface_r->description.msaa_level = msaa_level;

		window_surface_r->hidden.texture_2d = bb_texture;
		window_surface_r->hidden.rtv = rtv;
		window_surface_r->hidden.srv = srv;
		window_surface_r->hidden.dsv = nullptr;
		window_surface_r->hidden.swap_chain = swap_chain;
		window_surface_r->hidden.window_handle = window_handle;

		return window_surface_r;
	}

	void GPUBackend::swap_buffers(const Surface* window_surface)
	{
		if (window_surface != nullptr && window_surface->hidden.swap_chain)
			window_surface->hidden.swap_chain->Present(0, 0);
		else
			camy_error("Tried to swap buffers with invalid window surface");
	}

	void GPUBackend::clear_surface(const Surface* surface, const float* color, const float depth, const u32 stencil)
	{
		// Todo: debug if
		if (surface == nullptr)
		{
			camy_warning("Tried to clear a null surface");
			return;
		}

		if (surface->hidden.rtv != nullptr)
			m_context->ClearRenderTargetView(surface->hidden.rtv, color);
		if (surface->hidden.dsv != nullptr)
			m_context->ClearDepthStencilView(surface->hidden.dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
	}
}