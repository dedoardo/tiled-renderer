/* opengl4_impl_base.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

#if defined(CAMY_BACKEND_OPENGL4)

// OGL
#include <GL/glew.h>
#include <GL/wglew.h>
#undef near
#undef far
#undef min
#undef max

// camy
#include <camy/containers/dyn_linear_buffer.hpp>
#include <camy/containers/hash_map.hpp>
#include <camy/containers/dyn_array.hpp>
#include <camy/system.hpp>

namespace camy
{
    struct CAMY_API ConcurrentContextData
    {
        uint32 locked;
        ThreadID owner;
        HGLRC off_ctx;
    };

    struct CAMY_API RenderContextData
    {
        bool is_valid() const { return hdc != nullptr && render_ctx != nullptr; }

        HDC hdc = nullptr;
        HGLRC render_ctx = nullptr;
        Surface surface;
        ConcurrentContextData contexts[API::MAX_CONTEXTS];
        Atomic<uint32> avail_contexts;
    };

    // Container objects are not sharable between contexts in OGL,
    // Thus when compiling command lists we need to find a way to
    // request a delayed creation at flushing time (Just on the main context)
    // This is solved with hashing. Whenever a new container is specified the
    // handles to the contained resources are hashed and if the same one is found
    // then it's used otherwise a new one is created on the spot and will be used
    // next time. There is currently no deallocation.
    struct InitResRequest
    {
        enum class Type
        {
            Framebuffer,
            ProgramPipeline
        };

        Type type;
        uint64 hash;

        struct Framebuffer
        {
            uint16 render_targets[API::MAX_RENDER_TARGETS];
            uint16 depth_buffer;
        };

        struct ProgramPipeline
        {
            uint16 vertex_shader;
            uint16 pixel_shader;
        };

        union {
            Framebuffer framebuffer;
            ProgramPipeline program_pipeline;
        };
    };

    struct CAMY_API CommandListData
    {
        DynLinearBuffer command_buffer;
        DynArray<InitResRequest> init_requests;

        HashMap<GLuint> fbo_map;
        HashMap<GLuint> ppo_map;

        // Group in state
        uint16 cur_vertex_shader;
        uint16 cur_pixel_shader;
        GLenum cur_primitive_topology;
        uint16 cur_input_signature;
        uint16 cur_vertex_buffers[API::MAX_VERTEX_BUFFERS];
        uint16 cur_index_buffer;
    };

    namespace OpenGL4
    {
        CAMY_INLINE void flush_errors()
        {
            while (glGetError() != GL_NO_ERROR)
                ;
        }

        CAMY_INLINE bool has_errors()
        {
            GLenum error = glGetError();

            if (error == GL_NO_ERROR) return false;

            while (error != GL_NO_ERROR)
            {
                char* error_str;
                switch (error)
                {
                case GL_INVALID_OPERATION:
                    error_str = "INVALID_OPERATION";
                    break;
                case GL_INVALID_ENUM:
                    error_str = "INVALID_ENUM";
                    break;
                case GL_INVALID_VALUE:
                    error_str = "INVALID_VALUE";
                    break;
                case GL_OUT_OF_MEMORY:
                    error_str = "OUT_OF_MEMORY";
                    break;
                case GL_INVALID_FRAMEBUFFER_OPERATION:
                    error_str = "INVALID_FRAMEBUFFER_OPERATION";
                    break;
                }

                CL_ERR("Opengl4.5 Error: ", error_str);
                error = glGetError();
            }

            return true;
        }
    }
}

#endif