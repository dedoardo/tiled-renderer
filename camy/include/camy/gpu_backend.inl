// D3D11
#define NOMINMAX
#include <d3d11.h> // Todo : This should be removed, but i dont want to make the bind_*** not inline
#include <d3dcompiler.h>
#undef NOMINMAX
#undef near
#undef far

// camy
#include "pipeline_cache.hpp"
#include "cbuffer_system.hpp"

namespace camy
{
	template <typename Type>
	void GPUBackend::dispose(Type* ptr)
	{
		if (ptr != nullptr)
		{
			m_resources.deallocate(ptr);
		}
	}

	template <typename Type>
	void GPUBackend::safe_dispose(Type*& ptr)
	{
		if (ptr != nullptr)
		{
			m_resources.deallocate(ptr);
			ptr = nullptr;
		}
	}

	/*
		Might consider moving everything to a .cpp file
	*/
	namespace hidden
	{
#define camy_bind_warn_if_null(type, var, member, bind_point) if ((var) == nullptr || static_cast<type>(var)->hidden.member == nullptr) { camy_warning("Failed to bind at", bind_point); return; }

		static camy_inline void bind_sampler_vs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* sampler)
		{
			camy_bind_warn_if_null(const camy::Sampler*, sampler, sampler, shader_var.slot);
			context->VSSetSamplers(shader_var.slot, 1, &static_cast<const camy::Sampler*>(sampler)->hidden.sampler);
		}
	
		static camy_inline void bind_surface_vs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* surface)
		{
			camy_bind_warn_if_null(const camy::Surface*, surface, srv, shader_var.slot);
			context->VSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Surface*>(surface)->hidden.srv);
		}

		static camy_inline void bind_buffer_vs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* buffer)
		{
			camy_bind_warn_if_null(const camy::Buffer*, buffer, srv, shader_var.slot);
			context->VSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Buffer*>(buffer)->hidden.srv);
		}

		static camy_inline void bind_cbuffer_vs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* data)
		{
			//Ok now there are different possible cases, we can reuse the same cbuffer only if the upper pow2 sizes
			// are the same, meaning that data will fit there the same way. If this fails we need to request a new cbuffer
			// bind it and update it. Best case scenario render items usually have the same cbuffer ( say material ) set 
			// every time this way we don't need to rebind a new one, but the update problem still remains. This is where 
			// pointers kick in, we check if the previous constant buffer has been update ( data pointer) corresponds to this
			// one, if so then we are all set and don't need to do anything.
			// As you can not here im not checking for *ANY* cbuffer size the reason is if a cbuffer is already bound, but 
			// only a part of the data is actually going to be used ( thus the size of the data parameter is less than the current one
			// is not a problem, we avoid rebinding / updating at the cost of having bound a bigger constant buffer. Cleaner code too
			if (pc.cbuffer_cache[shader_var.slot] == nullptr ||
				pc.cbuffer_cache[shader_var.slot]->last_data != data)
			{
				auto shader{ pc.shaders[shader_var.shader_type] };

				// Getting best fitting constant buffer
				const auto cbuffer_size{ shader_var.size };
				auto new_cbuffer{ cbuffers[static_cast<u32>(shader_var.shader_type)].get(shader_var.slot, cbuffer_size) };

				// Updating it with new data
				D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
				context->Map(new_cbuffer->cbuffer->hidden.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
				std::memcpy(mapped_cbuffer.pData, data, cbuffer_size);
				context->Unmap(new_cbuffer->cbuffer->hidden.buffer, 0);

				context->VSSetConstantBuffers(shader_var.slot, 1, &new_cbuffer->cbuffer->hidden.buffer);
				
				// Saving it as current one
				pc.cbuffer_cache[shader_var.slot] = new_cbuffer;
				new_cbuffer->last_data = data;
			}
		}


		static camy_inline void bind_sampler_gs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* sampler)
		{
			camy_bind_warn_if_null(const camy::Sampler*, sampler, sampler, shader_var.slot);
			context->GSSetSamplers(shader_var.slot, 1, &static_cast<const camy::Sampler*>(sampler)->hidden.sampler);
		}

		static camy_inline void bind_surface_gs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* surface)
		{
			camy_bind_warn_if_null(const camy::Surface*, surface, srv, shader_var.slot);
			context->GSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Surface*>(surface)->hidden.srv);
		}

		static camy_inline void bind_buffer_gs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* buffer)
		{
			camy_bind_warn_if_null(const camy::Buffer*, buffer, srv, shader_var.slot);
			context->GSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Buffer*>(buffer)->hidden.srv);
		}

		static camy_inline void bind_cbuffer_gs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* data)
		{
			//Ok now there are different possible cases, we can reuse the same cbuffer only if the upper pow2 sizes
			// are the same, meaning that data will fit there the same way. If this fails we need to request a new cbuffer
			// bind it and update it. Best case scenario render items usually have the same cbuffer ( say material ) set 
			// every time this way we don't need to rebind a new one, but the update problem still remains. This is where 
			// pointers kick in, we check if the previous constant buffer has been update ( data pointer) corresponds to this
			// one, if so then we are all set and don't need to do anything.
			// As you can not here im not checking for *ANY* cbuffer size the reason is if a cbuffer is already bound, but 
			// only a part of the data is actually going to be used ( thus the size of the data parameter is less than the current one
			// is not a problem, we avoid rebinding / updating at the cost of having bound a bigger constant buffer. Cleaner code too
			if (pc.cbuffer_cache[shader_var.slot] == nullptr ||
				pc.cbuffer_cache[shader_var.slot]->last_data != data)
			{
				auto shader{ pc.shaders[shader_var.shader_type] };

				// Getting best fitting constant buffer
				const auto cbuffer_size{ shader_var.size };
				auto new_cbuffer{ cbuffers[static_cast<u32>(shader_var.shader_type)].get(shader_var.slot, cbuffer_size) };

				// Updating it with new data
				D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
				context->Map(new_cbuffer->cbuffer->hidden.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
				std::memcpy(mapped_cbuffer.pData, data, cbuffer_size);
				context->Unmap(new_cbuffer->cbuffer->hidden.buffer, 0);

				context->GSSetConstantBuffers(shader_var.slot, 1, &new_cbuffer->cbuffer->hidden.buffer);

				// Saving it as current one
				pc.cbuffer_cache[shader_var.slot] = new_cbuffer;
				new_cbuffer->last_data = data;
			}
		}


		static camy_inline void bind_sampler_ps(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* sampler)
		{
			camy_bind_warn_if_null(const camy::Sampler*, sampler, sampler, shader_var.slot);
			context->PSSetSamplers(shader_var.slot, 1, &static_cast<const camy::Sampler*>(sampler)->hidden.sampler);
		}

		static camy_inline void bind_surface_ps(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* surface)
		{
			camy_bind_warn_if_null(const camy::Surface*, surface, srv, shader_var.slot);
			context->PSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Surface*>(surface)->hidden.srv);
		}

		static camy_inline void bind_buffer_ps(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* buffer)
		{
			camy_bind_warn_if_null(const camy::Buffer*, buffer, srv, shader_var.slot);
			context->PSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Buffer*>(buffer)->hidden.srv);
		}

		static camy_inline void bind_cbuffer_ps(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* data)
		{
			//Ok now there are different possible cases, we can reuse the same cbuffer only if the upper pow2 sizes
			// are the same, meaning that data will fit there the same way. If this fails we need to request a new cbuffer
			// bind it and update it. Best case scenario render items usually have the same cbuffer ( say material ) set 
			// every time this way we don't need to rebind a new one, but the update problem still remains. This is where 
			// pointers kick in, we check if the previous constant buffer has been update ( data pointer) corresponds to this
			// one, if so then we are all set and don't need to do anything.
			// As you can not here im not checking for *ANY* cbuffer size the reason is if a cbuffer is already bound, but 
			// only a part of the data is actually going to be used ( thus the size of the data parameter is less than the current one
			// is not a problem, we avoid rebinding / updating at the cost of having bound a bigger constant buffer. Cleaner code too
			if (pc.cbuffer_cache[shader_var.slot] == nullptr ||
				pc.cbuffer_cache[shader_var.slot]->last_data != data)
			{
				auto shader{ pc.shaders[shader_var.shader_type] };

				// Getting best fitting constant buffer
				const auto cbuffer_size{ shader_var.size };
				auto new_cbuffer{ cbuffers[static_cast<u32>(shader_var.shader_type)].get(shader_var.slot, cbuffer_size) };

				// Updating it with new data
				D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
				context->Map(new_cbuffer->cbuffer->hidden.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
				std::memcpy(mapped_cbuffer.pData, data, cbuffer_size);
				context->Unmap(new_cbuffer->cbuffer->hidden.buffer, 0);

				context->PSSetConstantBuffers(shader_var.slot, 1, &new_cbuffer->cbuffer->hidden.buffer);

				// Saving it as current one
				pc.cbuffer_cache[shader_var.slot] = new_cbuffer;
				new_cbuffer->last_data = data;
			}
		}


		static camy_inline void bind_sampler_cs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* sampler)
		{
			camy_bind_warn_if_null(const camy::Sampler*, sampler, sampler, shader_var.slot);
			context->CSSetSamplers(shader_var.slot, 1, &static_cast<const camy::Sampler*>(sampler)->hidden.sampler);
		}

		static camy_inline void bind_surface_cs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* surface)
		{
			camy_bind_warn_if_null(const camy::Surface*, surface, srv, shader_var.slot);
			
			auto ss = static_cast<const camy::Surface*>(surface);

			if (shader_var.is_uav)
				context->CSSetUnorderedAccessViews(shader_var.slot, 1, &static_cast<const camy::Surface*>(surface)->hidden.uav, nullptr);
			else
				context->CSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Surface*>(surface)->hidden.srv);
		}

		static camy_inline void bind_buffer_cs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* buffer)
		{
			camy_bind_warn_if_null(const camy::Buffer*, buffer, srv, shader_var.slot);

			if (shader_var.is_uav)
				context->CSSetUnorderedAccessViews(shader_var.slot, 1, &static_cast<const camy::Buffer*>(buffer)->hidden.uav, nullptr);
			else
				context->CSSetShaderResources(shader_var.slot, 1, &static_cast<const camy::Buffer*>(buffer)->hidden.srv);
		}

		static camy_inline void bind_cbuffer_cs(ID3D11DeviceContext* context, CBufferSystem* cbuffers, PipelineCache& pc, ShaderVariable shader_var, const void* data)
		{
			//Ok now there are different possible cases, we can reuse the same cbuffer only if the upper pow2 sizes
			// are the same, meaning that data will fit there the same way. If this fails we need to request a new cbuffer
			// bind it and update it. Best case scenario render items usually have the same cbuffer ( say material ) set 
			// every time this way we don't need to rebind a new one, but the update problem still remains. This is where 
			// pointers kick in, we check if the previous constant buffer's update ( data pointer) corresponds to this
			// one, if so then we are all set and don't need to do anything.
			// As you can not here im not checking for *ANY* cbuffer size the reason is if a cbuffer is already bound, but 
			// only a part of the data is actually going to be used ( thus the size of the data parameter is less than the current one
			// is not a problem, we avoid rebinding / updating at the cost of having bound a bigger constant buffer. Cleaner code too
			if (pc.cbuffer_cache[shader_var.slot] == nullptr ||
				pc.cbuffer_cache[shader_var.slot]->last_data != data)
			{
				// Getting best fitting constant buffer
				const auto cbuffer_size{ shader_var.size };
				auto new_cbuffer{ cbuffers[static_cast<u32>(shader_var.shader_type)].get(shader_var.slot, cbuffer_size) };

				// Updating it with new data
				D3D11_MAPPED_SUBRESOURCE mapped_cbuffer;
				context->Map(new_cbuffer->cbuffer->hidden.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_cbuffer);
				std::memcpy(mapped_cbuffer.pData, data, cbuffer_size);
				context->Unmap(new_cbuffer->cbuffer->hidden.buffer, 0);

				context->CSSetConstantBuffers(shader_var.slot, 1, &new_cbuffer->cbuffer->hidden.buffer);

				// Saving it as current one
				pc.cbuffer_cache[shader_var.slot] = new_cbuffer;
				new_cbuffer->last_data = data;
			}
		}

		/*
			Has to respect the order of BindType and Shader::Type
		*/
		static void(*bind_lookup_table[])(ID3D11DeviceContext*, CBufferSystem*, PipelineCache&, ShaderVariable, const void*)
		{
			bind_sampler_vs,
			bind_surface_vs,
			bind_buffer_vs,
			bind_cbuffer_vs,

			bind_sampler_gs,
			bind_surface_gs,
			bind_buffer_gs,
			bind_cbuffer_gs,

			bind_sampler_ps,
			bind_surface_ps,
			bind_buffer_ps,
			bind_cbuffer_ps,

			bind_sampler_cs,
			bind_surface_cs,
			bind_buffer_cs,
			bind_cbuffer_cs,
		};
	}

	camy_inline void GPUBackend::set_parameters(const ParameterGroup& parameters, PipelineCache& pipeline_cache)
	{
		_m_prefetch(hidden::bind_lookup_table);

		for (auto i{ 0u }; i < parameters.num_parameters; ++i)
			set_parameter(parameters.parameters[i], pipeline_cache);
	}

	camy_inline void GPUBackend::set_parameter(const PipelineParameter& parameter, PipelineCache& pipeline_cache)
	{
		using namespace hidden;

		if (parameter.data == nullptr)
		{
			camy_warning("Failed to set parameter, invalid data in PipelineParameter"); // Todo : dump shader variable
			return;
		}

		if (parameter.shader_variable.valid == 0)
		{
			camy_warning("Invalid shader variable passed as parameter");
			return;
		}

		auto lookup_index{ parameter.shader_variable.shader_type * 4 + parameter.shader_variable.type };
		(*hidden::bind_lookup_table[lookup_index])(m_context, m_cbuffers, pipeline_cache, parameter.shader_variable, parameter.data);
	}


	camy_inline void GPUBackend::set_common_states(const CommonStates& common_states)
	{
		ID3D11RenderTargetView* rtvs[features::max_cachable_rts];
		std::memset(rtvs, 0, sizeof(ID3D11RenderTargetView*) * features::max_cachable_rts); // Could be done manually
		for (auto i{ 0u }; i < features::max_cachable_rts; ++i)
		{
			if (common_states.render_targets[i] != nullptr)
			{
				rtvs[i] = common_states.render_targets[i]->hidden.rtv;
				if (rtvs[i] == nullptr)
					camy_warning("Not properly created render target view bound at slot: ", i);
			}
		}

		ID3D11DepthStencilView* dsv{ nullptr };
		if (common_states.depth_buffer != nullptr)
		{
			dsv = common_states.depth_buffer->hidden.dsv;
			if (dsv == nullptr)
				camy_warning("Tried to bind non properly created depth buffer");
		}

		m_context->OMSetRenderTargets(features::max_cachable_rts, rtvs, dsv);

		// Todo: implement custom depth
		D3D11_VIEWPORT vp;
		vp.TopLeftX = common_states.viewport.left;
		vp.TopLeftY = common_states.viewport.top;
		vp.Width = common_states.viewport.right - common_states.viewport.left;
		vp.Height = common_states.viewport.bottom - common_states.viewport.top;
		vp.MinDepth = 0.f;
		vp.MaxDepth = 1.f;

		m_context->RSSetViewports(1, &vp);

		ID3D11BlendState* blend_state{ nullptr };
		if (common_states.blend_state != nullptr)
		{
			blend_state = common_states.blend_state->hidden.state;
			if (blend_state == nullptr)
				camy_warning("Tried to bind non properly created blend state");
		}
		m_context->OMSetBlendState(blend_state, nullptr, 0xFFFFFFFF);


		ID3D11RasterizerState* rasterizer_state{ nullptr };
		if (common_states.rasterizer_state != nullptr)
		{
			rasterizer_state = common_states.rasterizer_state->hidden.state;
			if (rasterizer_state == nullptr)
				camy_warning("Tried to bind non properly creatd rasterizer state");
		}
		m_context->RSSetState(rasterizer_state);

		ID3D11DepthStencilState* depth_stencil_state{ nullptr };
		if (common_states.depth_stencil_state != nullptr)
		{
			depth_stencil_state = common_states.depth_stencil_state->hidden.state;
			if (depth_stencil_state == nullptr)
				camy_warning("Tried to bind non properly create depth stencil state");
		}
		m_context->OMSetDepthStencilState(depth_stencil_state, 0);
	}

	camy_inline void GPUBackend::set_default_common_states()
	{
		ID3D11RenderTargetView* rtv_upload[features::max_cachable_rts]{ nullptr, nullptr };
		m_context->OMSetRenderTargets(features::max_cachable_rts, rtv_upload, nullptr);
		m_context->RSSetState(nullptr);
		m_context->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
		m_context->OMSetDepthStencilState(nullptr, 0);
	}

	camy_inline void GPUBackend::unbind_dependency(const Dependency& dependency)
	{
		// No lookup table here, not really needed, called at the end of each render queue
		if (dependency.valid == 0)
		{
			camy_warning("Invalid dependency"); // Todo: dump dependency
			return;
		}

		// Todo: only unbinding srvs for now
		if (dependency.type != static_cast<u32>(BindType::Surface) &&
			dependency.type != static_cast<u32>(BindType::Buffer))
		{
			// Warning currently suppressed because execute(compute) feeds all the parameters here for unbinding
			// and cbuffers can be present thus polluting the output
			//camy_warning("Invalid dependency bindtype, only srvs are supported atm ( surface / buffer ) ");
			return;
		}

		ID3D11ShaderResourceView* null_srv{ nullptr };
		ID3D11UnorderedAccessView* null_uav{ nullptr };
	
		if (dependency.shader_type == static_cast<u32>(Shader::Type::Vertex))
			m_context->VSSetShaderResources(dependency.slot, 1, &null_srv);
		else if (dependency.shader_type == static_cast<u32>(Shader::Type::Pixel))
			m_context->PSSetShaderResources(dependency.slot, 1, &null_srv);
		else if (dependency.shader_type == static_cast<u32>(Shader::Type::Geometry))
			m_context->GSSetShaderResources(dependency.slot, 1, &null_srv);
		else if (dependency.shader_type == static_cast<u32>(Shader::Type::Compute))
		{
			if (dependency.is_uav)
				m_context->CSSetUnorderedAccessViews(dependency.slot, 1, &null_uav, nullptr);
			else
				m_context->CSSetShaderResources(dependency.slot, 1, &null_srv);
		}
	}
}