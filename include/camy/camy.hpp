/* camy.hpp
*
* Copyright (C) 2017 Edoardo Dominici
*
* This software may be modified and distributed under the terms
* of the MIT license.  See the LICENSE file for details.
*/
#pragma once

// camy
#include <camy/containers/static_string.hpp>
#include <camy/core.hpp>
#include <camy/graphics.hpp>
#include <camy/math.hpp>
#include <camy/system.hpp>

namespace camy
{
    //! Window description
    struct CAMY_API WindowDesc
    {
        enum Style : uint32
        {
            Default,    // Borders and such
            Popup,      // ClientArea == WindowArea
            Fullscreen, // Popup + Screen set to the specified resolution ( or closest one )
            None        // No window will be created
        };

        /*!
        void* should be cast to ProcData that contains data depending on the
        platform. It's here as a "raw" interface.
        void* is a pointer to a struct containing all the parameters in the same order

        struct WinProcData
        {
        HWND hwnd;
        UINT msg;
        LPARAM lparam;
        WPARAM wparam;
        };
        !*/
        using Proc = void (*)(void*);

        const char8* name;
        Style style = Style::Default;
        uint16 width = 0;
        uint16 height = 0;
        Proc proc = nullptr;
    };

    //! Startup parameters when calling API::startup()
    struct CAMY_API StartupInfo
    {
        //! Description of the optional window to be created alongside initialization
        WindowDesc window;

        //! If true camy assumes that the backbuffer is srgb (if WindowDesc is valid the window)
        //! will be created accordingly.
        bool is_srgb = true;

        //! level of msaa, 1x, 2x, 4x, 8x
        uint8 msaa = 1;

        //! Index of the preferred device to be used. Otherwise the first one supporting the
        //! backend is used
        uint8 preferred_device = 0;

        ContextID render_ctx;
    };

    //! Values currently being used by camy. Can be compared w/ StartupInfo
    //! to make sure that all requested features are present. A warning should have
    //! been posted if something went wrong.
    struct CAMY_API RuntimeInfo
    {
        //! Platform specific window handle.
        //! Win32 :: Can be safely cast to HWND
        void* whandle = nullptr;

        //! Width in pixels of the created window. Corresponds to the window area.
        //! if style at creation was set to Style::Fullscreen then the closest matching
        //! resolution supported by the screen is used.
        //! -1 if style was Style::None as no window was created
        sint16 width = -1;

        //! Height in pixels of the created window. See <width>
        sint16 height = -1;

        //! Msaa level of the backbuffer. -1 if no window
        sint8 msaa = -1;

        //! Index of the device being used. -1 if creation was not successful
        sint8 device_in_use = -1;

        //! String representing the graphics backend being used and its version
        StaticString<32> backend = "<none>";

        //! [optional] String representing the vendor of the gpu the device represents
        StaticString<32> gpu_vendor = "<none>";

        //! [optional] String representing the name of the gpu the device represents
        StaticString<32> gpu_name = "<none>";

        //! [optional] Dedicated megabytes of graphic memory supported by the device
        sint32 dedicated_memory = -1;
    };

    class RenderContext;

    // Free functions
    namespace API
    {
        /*!
        Creates a new rendering context and associated window for the platform.
        To retrieve the native handle, simply call get_context_info()
        results vary depending on the platform
        !*/
        CAMY_API bool startup(const StartupInfo& info);

        /*!
        Shuts down the context releasing all the resources
        !*/
        CAMY_API void shutdown();

        /*!
        Returns a RenderContext create by the last call to startup. There can
        only be one rendercontext at a time. ( OpenGL Note: multithreading is still viable,
        see RenderContext::aquire() RenderContext::release() )
        !*/
        CAMY_API RenderContext& render_context();
        CAMY_API RenderContext& rc();

        /*!
        Runtime info for camy, can be checked against StartupInfo to check which
        features have been enabled and which not
        !*/
        CAMY_API const RuntimeInfo& runtime_info();
        CAMY_API const RuntimeInfo& info();

        enum class CAMY_API Query
        {
            ConstantByteSize,
            MinConstantsPerUpdate,
            ShaderMatrixOrder,
        };

        enum class CAMY_API ShaderMatrixOrder : rsize
        {
            RowMajor,
            ColMajor
        };

        CAMY_API rsize query(Query query);

        CAMY_API float4x4 to_shader_order(const float4x4& mat);
    }
}