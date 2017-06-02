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

namespace camy
{
    namespace OpenGL4
    {
        namespace Cmd
        {
#define new_cmd(name, id)                                                                          \
    \
struct name                                                                                        \
    {                                                                                              \
        static const ::camy::rsize ID = id;                                                        \
    };

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

            template <typename T1, typename T2, typename... Ts>
            CAMY_INLINE void append_rec(CommandListData& data, T1&& arg1, T2&& arg2, Ts&&... args)
            {
                data.command_buffer.append<T1>(arg1);
                append_rec(data, std::forward<T2>(arg2), std::forward<Ts>(args)...);
            }

            template <typename T, typename... Args>
            CAMY_INLINE void append(CommandListData& data, Args&&... args)
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

#define CAMY_CMD_IMPL(name)                                                                        \
    CAMY_INLINE void cmd_##name(RenderContextData& rc_data, CommandListData& cl_data,              \
                                const byte*& cur)
#define CAMY_CMD_READ(name, type) type name = read<type>(cur);
#define CAMY_CMD_CHECK(name)                                                                       \
    if (OpenGL4::has_errors()) CL_ERR("Failed to " #name ", see above for more details");

            CAMY_CMD_IMPL(BindProgramPipeline)
            {
                CAMY_CMD_READ(hash, uint64);
                GLuint ppo = *cl_data.ppo_map[hash];
                CAMY_ASSERT(ppo != 0);
                glBindProgramPipeline(ppo);
                CAMY_CMD_CHECK(glBindProgramPipeline);
            }

            CAMY_CMD_IMPL(BindFramebuffer)
            {
                CAMY_CMD_READ(target, GLenum);
                CAMY_CMD_READ(hash, uint64);
                GLuint fbo = *cl_data.fbo_map[hash];
                glBindFramebuffer(target, fbo);
                CAMY_CMD_CHECK(glBindFramebuffer);
            }

            CAMY_CMD_IMPL(BindInputSignature)
            {
                CAMY_CMD_READ(handle, HResource);
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
                CAMY_CMD_CHECK(glBindVertexArray);
            }

            CAMY_CMD_IMPL(Enable)
            {
                CAMY_CMD_READ(cap, GLenum);
                glEnable(cap);
                CAMY_CMD_CHECK(glEnable);
            }

            CAMY_CMD_IMPL(Disable)
            {
                CAMY_CMD_READ(cap, GLenum);
                glDisable(cap);
                CAMY_CMD_CHECK(glDisable);
            }

            CAMY_CMD_IMPL(PolygonMode)
            {
                CAMY_CMD_READ(face, GLenum);
                CAMY_CMD_READ(mode, GLenum);
                glPolygonMode(face, mode);
                CAMY_CMD_CHECK(glPolygonMode);
            }

            CAMY_CMD_IMPL(CullFace)
            {
                CAMY_CMD_READ(mode, GLenum);
                glCullFace(mode);
                CAMY_CMD_CHECK(glCullFace);
            }

            CAMY_CMD_IMPL(PolygonOffsetClamp)
            {
                CAMY_CMD_READ(factor, GLfloat);
                CAMY_CMD_READ(units, GLfloat);
                CAMY_CMD_READ(clamp, GLfloat);
                glPolygonOffsetClampEXT(factor, units, clamp);
                CAMY_CMD_CHECK(glPolygonOffsetClampEXT);
            }

            CAMY_CMD_IMPL(DepthFunc)
            {
                CAMY_CMD_READ(func, GLenum);
                glDepthFunc(func);
                CAMY_CMD_CHECK(glDepthFunc);
            }

            CAMY_CMD_IMPL(Viewport)
            {
                CAMY_CMD_READ(x, GLint);
                CAMY_CMD_READ(y, GLint);
                CAMY_CMD_READ(width, GLsizei);
                CAMY_CMD_READ(height, GLsizei);
                glViewport(x, y, width, height);
                CAMY_CMD_CHECK(glViewport);
            }

            CAMY_CMD_IMPL(DepthRange)
            {
                CAMY_CMD_READ(n, GLfloat);
                CAMY_CMD_READ(f, GLfloat);
                glDepthRangef(n, f);
                CAMY_CMD_CHECK(glDepthRange);
            }

            CAMY_CMD_IMPL(BindBuffer)
            {
                CAMY_CMD_READ(target, GLenum);
                CAMY_CMD_READ(buffer, GLuint);
                glBindBuffer(target, buffer);
                CAMY_CMD_CHECK(glBindBuffer);
            }

            CAMY_CMD_IMPL(BindBufferBase)
            {
                CAMY_CMD_READ(target, GLenum);
                CAMY_CMD_READ(index, GLuint);
                CAMY_CMD_READ(buffer, GLuint);
                glBindBufferBase(target, index, buffer);
                CAMY_CMD_CHECK(glBindBufferBase);
            }

            CAMY_CMD_IMPL(BindBufferRange)
            {
                CAMY_CMD_READ(target, GLenum);
                CAMY_CMD_READ(index, GLuint);
                CAMY_CMD_READ(buffer, GLuint);
                CAMY_CMD_READ(offset, GLintptr);
                CAMY_CMD_READ(size, GLsizeiptr);
                glBindBufferRange(target, index, buffer, offset, size);
                CAMY_CMD_CHECK(glBindBufferRange);
            }

            CAMY_CMD_IMPL(ActiveTexture)
            {
                CAMY_CMD_READ(texture, GLenum);
                glActiveTexture(texture);
                CAMY_CMD_CHECK(glActiveTexture);
            }

            CAMY_CMD_IMPL(BindTexture)
            {
                CAMY_CMD_READ(target, GLenum);
                CAMY_CMD_READ(texture, GLuint);
                glBindTexture(target, texture);
                CAMY_CMD_CHECK(glBindTexture);
            }

            CAMY_CMD_IMPL(BindSampler)
            {
                CAMY_CMD_READ(unit, GLuint);
                CAMY_CMD_READ(sampler, GLuint);
                glBindSampler(unit, sampler);
                CAMY_CMD_CHECK(glBindSampler);
            }

            CAMY_CMD_IMPL(BindVertexArray)
            {
                CAMY_CMD_READ(array, GLuint);
                glBindVertexArray(array);
                CAMY_CMD_CHECK(glBindVertexArray);
            }

            CAMY_CMD_IMPL(BindVertexBuffer)
            {
                CAMY_CMD_READ(binding_index, GLuint);
                CAMY_CMD_READ(buffer, GLuint);
                CAMY_CMD_READ(offset, GLintptr);
                CAMY_CMD_READ(stride, GLintptr);
                glBindVertexBuffer(binding_index, buffer, offset, stride);
                CAMY_CMD_CHECK(glBindVertexBuffer);
            }

            CAMY_CMD_IMPL(DrawArrays)
            {
                CAMY_CMD_READ(mode, GLenum);
                CAMY_CMD_READ(first, GLint);
                CAMY_CMD_READ(count, GLsizei);
                glDrawArrays(mode, first, count);
                CAMY_CMD_CHECK(glDrawArrays);
            }

            CAMY_CMD_IMPL(DrawElementsBaseVertex)
            {
                CAMY_CMD_READ(mode, GLenum);
                CAMY_CMD_READ(count, GLsizei);
                CAMY_CMD_READ(type, GLenum);
                CAMY_CMD_READ(indices, GLvoid*);
                CAMY_CMD_READ(base_vertex, GLint);
                glDrawElementsBaseVertex(mode, count, type, indices, base_vertex);
                CAMY_CMD_CHECK(glDrawElementsBaseVertex);
            }

            CAMY_CMD_IMPL(DrawArraysInstancedBaseInstance)
            {
                CAMY_CMD_READ(mode, GLenum);
                CAMY_CMD_READ(first, GLint);
                CAMY_CMD_READ(count, GLsizei);
                CAMY_CMD_READ(prim_count, GLsizei);
                CAMY_CMD_READ(base_instance, GLuint);
                glDrawArraysInstancedBaseInstance(mode, first, count, prim_count, base_instance);
                CAMY_CMD_CHECK(glDrawArrraysInstancedBaseInstance);
            }

            CAMY_CMD_IMPL(DrawElementsInstancedBaseVertex)
            {
                CAMY_CMD_READ(mode, GLenum);
                CAMY_CMD_READ(count, GLsizei);
                CAMY_CMD_READ(type, GLenum);
                CAMY_CMD_READ(indices, GLvoid*);
                CAMY_CMD_READ(prim_count, GLsizei);
                CAMY_CMD_READ(base_vertex, GLint);
                glDrawElementsInstancedBaseVertex(mode, count, type, indices, prim_count,
                                                  base_vertex);
                CAMY_CMD_CHECK(glDrawElementsInstancedBaseVertex);
            }

            CAMY_CMD_IMPL(ClearTexImage)
            {
                CAMY_CMD_READ(texture, GLuint);
                CAMY_CMD_READ(level, GLint);
                CAMY_CMD_READ(format, GLenum);
                CAMY_CMD_READ(type, GLenum);
                CAMY_CMD_READ(byte_size, uint8);
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
                }
                cur += byte_size;
                CAMY_CMD_CHECK(glBindFrameBuffer);
            }

            void (*cmd_ftbl[])(RenderContextData&, CommandListData&, const byte*&);
        }
    }
}

#endif