/* opengl4_command_list.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#if defined(CAMY_OS_WINDOWS) && defined(CAMY_BACKEND_OPENGL4)

// camy
#include <camy/core.hpp>

namespace camy
{
	extern bool init_input_signature(InputSignature& is);
}

namespace camy { namespace OpenGL4
{
	namespace Cmd
	{
#define new_cmd(name, id)\
struct name { static const ::camy::rsize ID = id; };	

		new_cmd(BindProgramPipeline, 0);
		new_cmd(BindFramebuffer, 1);
		new_cmd(BindInputSignature, 2);
		new_cmd(Enable, 3);
		new_cmd(Disable, 4);
		new_cmd(PolygonMode, 5);
		new_cmd(CullFace, 6);
		new_cmd(PolygonOffsetClamp, 7);
		new_cmd(DepthFunc, 8);
		new_cmd(Viewport, 9);
		new_cmd(DepthRange, 10);
		new_cmd(BindBuffer, 11);
		new_cmd(BindBufferBase, 12);
		new_cmd(BindBufferRange, 13);
		new_cmd(ActiveTexture, 14);
		new_cmd(BindTexture, 15);
		new_cmd(BindSampler, 16);
		new_cmd(BindVertexArray, 17);
		new_cmd(BindVertexBuffer, 18);
		new_cmd(DrawArrays, 19);
		new_cmd(DrawElementsBaseVertex, 20);
		new_cmd(DrawArraysInstancedBaseInstance, 21);
		new_cmd(DrawElementsInstancedBaseVertex, 22);
		new_cmd(ClearTexImage, 23);

		template <typename T>
		CAMY_INLINE void append_rec(CommandListData& data, T&& arg)
		{
			data.command_buffer.append<T>(arg);
		}

		template <typename T1, typename T2, typename ...Ts>
		CAMY_INLINE void append_rec(CommandListData& data, T1&& arg1, T2&& arg2, Ts&& ...args)
		{
			data.command_buffer.append<T1>(arg1);
			append_rec(data, std::forward<T2>(arg2), std::forward<Ts>(args)...);
		}

		template <typename T, typename ...Args>
		CAMY_INLINE void append(CommandListData& data, Args&& ...args)
		{
			data.command_buffer.append<uint16>(T::ID);
			append_rec(data, std::forward<Args>(args)...);
		}

		template <typename T>
		CAMY_INLINE const T& read(const byte*& cur)
		{
			const T& ret = *((const T*)cur);
			cur += sizeof(T);
			return ret;
		}

#define cmd_impl(name) CAMY_INLINE void cmd_##name(RenderContextData& rc_data, CommandListData& cl_data, const byte*& cur)
#define cmd_read(name, type) type name = read<type>(cur);

		cmd_impl(BindProgramPipeline)
		{
			cmd_read(hash, uint64);
			GLuint ppo = *cl_data.ppo_map[hash];
			CAMY_ASSERT(ppo != 0);
			glBindProgramPipeline(ppo);
		}

		cmd_impl(BindFramebuffer)
		{
			cmd_read(target, GLenum);
			cmd_read(hash, uint64);
			GLuint fbo = *cl_data.fbo_map[hash];
			glBindFramebuffer(target, fbo);
			gl_err();
		}

		cmd_impl(BindInputSignature)
		{
			cmd_read(handle, HResource);
			InputSignature& is = API::rc().get_input_signature(handle);
			if (is.native.vao == 0)
			{
				if (!init_input_signature(is))
				{
					// ERR
					return;
				}
			}
			glBindVertexArray(is.native.vao);
		}

		cmd_impl(Enable)
		{
			cmd_read(cap, GLenum);
			glEnable(cap);
		}

		cmd_impl(Disable)
		{
			cmd_read(cap, GLenum);
			glDisable(cap);
		}

		cmd_impl(PolygonMode)
		{
			cmd_read(face, GLenum);
			cmd_read(mode, GLenum);
			glPolygonMode(face, mode);
		}

		cmd_impl(CullFace)
		{
			cmd_read(mode, GLenum);
			glCullFace(mode);
		}

		cmd_impl(PolygonOffsetClamp)
		{
			cmd_read(factor, GLfloat);
			cmd_read(units, GLfloat);
			cmd_read(clamp, GLfloat);
			glPolygonOffsetClampEXT(factor, units, clamp);
		}

		cmd_impl(DepthFunc)
		{
			cmd_read(func, GLenum);
			glDepthFunc(func);
		}

		cmd_impl(Viewport)
		{
			cmd_read(x, GLint);
			cmd_read(y, GLint);
			cmd_read(width, GLsizei);
			cmd_read(height, GLsizei);
			glViewport(x, y, width, height);
		}

		cmd_impl(DepthRange)
		{
			cmd_read(n, GLfloat);
			cmd_read(f, GLfloat);
			glDepthRangef(n, f);
		}

		cmd_impl(BindBuffer)
		{
			cmd_read(target, GLenum);
			cmd_read(buffer, GLuint);
			glBindBuffer(target, buffer);
		}

		cmd_impl(BindBufferBase)
		{
			cmd_read(target, GLenum);
			cmd_read(index, GLuint);
			cmd_read(buffer, GLuint);
			glBindBufferBase(target, index, buffer);
		}

		cmd_impl(BindBufferRange)
		{
			cmd_read(target, GLenum);
			cmd_read(index, GLuint);
			cmd_read(buffer, GLuint);
			cmd_read(offset, GLintptr);
			cmd_read(size, GLsizeiptr);
			glBindBufferRange(target, index, buffer, offset, size);
		}

		cmd_impl(ActiveTexture)
		{
			cmd_read(texture, GLenum);
			glActiveTexture(texture);
		}

		cmd_impl(BindTexture)
		{
			cmd_read(target, GLenum);
			cmd_read(texture, GLuint);
			glBindTexture(target, texture);
		}

		cmd_impl(BindSampler)
		{
			cmd_read(unit, GLuint);
			cmd_read(sampler, GLuint);
			glBindSampler(unit, sampler);
		}

		cmd_impl(BindVertexArray)
		{
			cmd_read(array, GLuint);
			glBindVertexArray(array);
		}

		cmd_impl(BindVertexBuffer)
		{
			cmd_read(binding_index, GLuint);
			cmd_read(buffer, GLuint);
			cmd_read(offset, GLintptr);
			cmd_read(stride, GLintptr);
			glBindVertexBuffer(binding_index, buffer, offset, stride);
		}

		cmd_impl(DrawArrays)
		{
			cmd_read(mode, GLenum);
			cmd_read(first, GLint);
			cmd_read(count, GLsizei);
			glDrawArrays(mode, first, count);
		}

		cmd_impl(DrawElementsBaseVertex)
		{
			cmd_read(mode, GLenum);
			cmd_read(count, GLsizei);
			cmd_read(type, GLenum);
			cmd_read(indices, GLvoid*);
			cmd_read(base_vertex, GLint);
			glDrawElementsBaseVertex(mode, count, type, indices, base_vertex);
		}

		cmd_impl(DrawArraysInstancedBaseInstance)
		{
			cmd_read(mode, GLenum);
			cmd_read(first, GLint);
			cmd_read(count, GLsizei);
			cmd_read(prim_count, GLsizei);
			cmd_read(base_instance, GLuint);
			glDrawArraysInstancedBaseInstance(mode, first, count, prim_count, base_instance);
		}

		cmd_impl(DrawElementsInstancedBaseVertex)
		{
			cmd_read(mode, GLenum);
			cmd_read(count, GLsizei);
			cmd_read(type, GLenum);
			cmd_read(indices, GLvoid*);
			cmd_read(prim_count, GLsizei);
			cmd_read(base_vertex, GLint);
			glDrawElementsInstancedBaseVertex(mode, count, type, indices, prim_count, base_vertex);
		}

		cmd_impl(ClearTexImage)
		{
			cmd_read(texture, GLuint);
			cmd_read(level, GLint);
			cmd_read(format, GLenum);
			cmd_read(type, GLenum);
			cmd_read(byte_size, uint8);
			if (texture != 0)
				glClearTexImage(texture, level, format, type, cur);
			else
			{
				GLuint old_fbo;
				glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&old_fbo);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				float4& val = *((float4*)cur);
				glClearColor(val.x, val.y, val.z, val.w);
				glClear(GL_COLOR_BUFFER_BIT);
				glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
				gl_err();
			}
			cur += byte_size;
			gl_err();
		}

		void(*cmd_ftbl[])(RenderContextData&, CommandListData&, const byte*&);
	}
}
}

#endif