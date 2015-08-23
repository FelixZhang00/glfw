//========================================================================
// GLFW 3.2 WGL - www.glfw.org
//------------------------------------------------------------------------
// Copyright (c) 2002-2006 Marcus Geelnard
// Copyright (c) 2006-2010 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would
//    be appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not
//    be misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source
//    distribution.
//
//========================================================================

#include "internal.h"

#include <stdlib.h>
#include <malloc.h>
#include <assert.h>


// Initialize WGL-specific extensions
//
static void initWGLExtensions(_GLFWcontext* context)
{
    // Functions for WGL_EXT_extension_string
    // NOTE: These are needed by _glfwPlatformExtensionSupported
    context->wgl.GetExtensionsStringEXT = (PFNWGLGETEXTENSIONSSTRINGEXTPROC)
        _glfw_wglGetProcAddress("wglGetExtensionsStringEXT");
    context->wgl.GetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)
        _glfw_wglGetProcAddress("wglGetExtensionsStringARB");

    // Functions for WGL_ARB_create_context
    context->wgl.CreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
        _glfw_wglGetProcAddress("wglCreateContextAttribsARB");

    // Functions for WGL_EXT_swap_control
    context->wgl.SwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        _glfw_wglGetProcAddress("wglSwapIntervalEXT");

    // Functions for WGL_ARB_pixel_format
    context->wgl.GetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)
        _glfw_wglGetProcAddress("wglGetPixelFormatAttribivARB");

    // This needs to include every extension used below except for
    // WGL_ARB_extensions_string and WGL_EXT_extensions_string
    context->wgl.ARB_multisample =
        _glfwPlatformExtensionSupported("WGL_ARB_multisample");
    context->wgl.ARB_framebuffer_sRGB =
        _glfwPlatformExtensionSupported("WGL_ARB_framebuffer_sRGB");
    context->wgl.EXT_framebuffer_sRGB =
        _glfwPlatformExtensionSupported("WGL_EXT_framebuffer_sRGB");
    context->wgl.ARB_create_context =
        _glfwPlatformExtensionSupported("WGL_ARB_create_context");
    context->wgl.ARB_create_context_profile =
        _glfwPlatformExtensionSupported("WGL_ARB_create_context_profile");
    context->wgl.EXT_create_context_es2_profile =
        _glfwPlatformExtensionSupported("WGL_EXT_create_context_es2_profile");
    context->wgl.ARB_create_context_robustness =
        _glfwPlatformExtensionSupported("WGL_ARB_create_context_robustness");
    context->wgl.EXT_swap_control =
        _glfwPlatformExtensionSupported("WGL_EXT_swap_control");
    context->wgl.ARB_pixel_format =
        _glfwPlatformExtensionSupported("WGL_ARB_pixel_format");
    context->wgl.ARB_context_flush_control =
        _glfwPlatformExtensionSupported("WGL_ARB_context_flush_control");
}

// Returns the specified attribute of the specified pixel format
//
static int getPixelFormatAttrib(_GLFWcontext* context, int pixelFormat, int attrib)
{
    int value = 0;

    assert(context->wgl.ARB_pixel_format);

    if (!context->wgl.GetPixelFormatAttribivARB(context->wgl.dc,
                                                pixelFormat,
                                                0, 1, &attrib, &value))
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "WGL: Failed to retrieve pixel format attribute %i",
                        attrib);
        return 0;
    }

    return value;
}

// Return a list of available and usable framebuffer configs
//
static int choosePixelFormat(_GLFWcontext* context,
                             const _GLFWfbconfig* desired,
                             int* result)
{
    _GLFWfbconfig* usableConfigs;
    const _GLFWfbconfig* closest;
    int i, nativeCount, usableCount;

    if (context->wgl.ARB_pixel_format)
    {
        nativeCount = getPixelFormatAttrib(context,
                                           1,
                                           WGL_NUMBER_PIXEL_FORMATS_ARB);
    }
    else
    {
        nativeCount = DescribePixelFormat(context->wgl.dc,
                                          1,
                                          sizeof(PIXELFORMATDESCRIPTOR),
                                          NULL);
    }

    usableConfigs = calloc(nativeCount, sizeof(_GLFWfbconfig));
    usableCount = 0;

    for (i = 0;  i < nativeCount;  i++)
    {
        const int n = i + 1;
        _GLFWfbconfig* u = usableConfigs + usableCount;

        if (context->wgl.ARB_pixel_format)
        {
            // Get pixel format attributes through "modern" extension

            if (!getPixelFormatAttrib(context, n, WGL_SUPPORT_OPENGL_ARB) ||
                !getPixelFormatAttrib(context, n, WGL_DRAW_TO_WINDOW_ARB))
            {
                continue;
            }

            if (getPixelFormatAttrib(context, n, WGL_PIXEL_TYPE_ARB) !=
                WGL_TYPE_RGBA_ARB)
            {
                continue;
            }

            if (getPixelFormatAttrib(context, n, WGL_ACCELERATION_ARB) ==
                 WGL_NO_ACCELERATION_ARB)
            {
                continue;
            }

            u->redBits = getPixelFormatAttrib(context, n, WGL_RED_BITS_ARB);
            u->greenBits = getPixelFormatAttrib(context, n, WGL_GREEN_BITS_ARB);
            u->blueBits = getPixelFormatAttrib(context, n, WGL_BLUE_BITS_ARB);
            u->alphaBits = getPixelFormatAttrib(context, n, WGL_ALPHA_BITS_ARB);

            u->depthBits = getPixelFormatAttrib(context, n, WGL_DEPTH_BITS_ARB);
            u->stencilBits = getPixelFormatAttrib(context, n, WGL_STENCIL_BITS_ARB);

            u->accumRedBits = getPixelFormatAttrib(context, n, WGL_ACCUM_RED_BITS_ARB);
            u->accumGreenBits = getPixelFormatAttrib(context, n, WGL_ACCUM_GREEN_BITS_ARB);
            u->accumBlueBits = getPixelFormatAttrib(context, n, WGL_ACCUM_BLUE_BITS_ARB);
            u->accumAlphaBits = getPixelFormatAttrib(context, n, WGL_ACCUM_ALPHA_BITS_ARB);

            u->auxBuffers = getPixelFormatAttrib(context, n, WGL_AUX_BUFFERS_ARB);

            if (getPixelFormatAttrib(context, n, WGL_STEREO_ARB))
                u->stereo = GLFW_TRUE;
            if (getPixelFormatAttrib(context, n, WGL_DOUBLE_BUFFER_ARB))
                u->doublebuffer = GLFW_TRUE;

            if (context->wgl.ARB_multisample)
                u->samples = getPixelFormatAttrib(context, n, WGL_SAMPLES_ARB);

            if (context->wgl.ARB_framebuffer_sRGB ||
                context->wgl.EXT_framebuffer_sRGB)
            {
                if (getPixelFormatAttrib(context, n, WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB))
                    u->sRGB = GLFW_TRUE;
            }
        }
        else
        {
            PIXELFORMATDESCRIPTOR pfd;

            // Get pixel format attributes through legacy PFDs

            if (!DescribePixelFormat(context->wgl.dc,
                                     n,
                                     sizeof(PIXELFORMATDESCRIPTOR),
                                     &pfd))
            {
                continue;
            }

            if (!(pfd.dwFlags & PFD_DRAW_TO_WINDOW) ||
                !(pfd.dwFlags & PFD_SUPPORT_OPENGL))
            {
                continue;
            }

            if (!(pfd.dwFlags & PFD_GENERIC_ACCELERATED) &&
                (pfd.dwFlags & PFD_GENERIC_FORMAT))
            {
                continue;
            }

            if (pfd.iPixelType != PFD_TYPE_RGBA)
                continue;

            u->redBits = pfd.cRedBits;
            u->greenBits = pfd.cGreenBits;
            u->blueBits = pfd.cBlueBits;
            u->alphaBits = pfd.cAlphaBits;

            u->depthBits = pfd.cDepthBits;
            u->stencilBits = pfd.cStencilBits;

            u->accumRedBits = pfd.cAccumRedBits;
            u->accumGreenBits = pfd.cAccumGreenBits;
            u->accumBlueBits = pfd.cAccumBlueBits;
            u->accumAlphaBits = pfd.cAccumAlphaBits;

            u->auxBuffers = pfd.cAuxBuffers;

            if (pfd.dwFlags & PFD_STEREO)
                u->stereo = GLFW_TRUE;
            if (pfd.dwFlags & PFD_DOUBLEBUFFER)
                u->doublebuffer = GLFW_TRUE;
        }

        u->wgl = n;
        usableCount++;
    }

    if (!usableCount)
    {
        _glfwInputError(GLFW_API_UNAVAILABLE,
                        "WGL: The driver does not appear to support OpenGL");

        free(usableConfigs);
        return GLFW_FALSE;
    }

    closest = _glfwChooseFBConfig(desired, usableConfigs, usableCount);
    if (!closest)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "WGL: Failed to find a suitable pixel format");

        free(usableConfigs);
        return GLFW_FALSE;
    }

    *result = closest->wgl;
    free(usableConfigs);

    return GLFW_TRUE;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW internal API                      //////
//////////////////////////////////////////////////////////////////////////

// Initialize WGL
//
int _glfwInitContextAPI(void)
{
    if (!_glfwCreateContextTLS())
        return GLFW_FALSE;

    _glfw.wgl.opengl32.instance = LoadLibraryW(L"opengl32.dll");
    if (!_glfw.wgl.opengl32.instance)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR, "WGL: Failed to load opengl32.dll");
        return GLFW_FALSE;
    }

    _glfw.wgl.opengl32.CreateContext = (WGLCREATECONTEXT_T)
        GetProcAddress(_glfw.wgl.opengl32.instance, "wglCreateContext");
    _glfw.wgl.opengl32.DeleteContext = (WGLDELETECONTEXT_T)
        GetProcAddress(_glfw.wgl.opengl32.instance, "wglDeleteContext");
    _glfw.wgl.opengl32.GetProcAddress = (WGLGETPROCADDRESS_T)
        GetProcAddress(_glfw.wgl.opengl32.instance, "wglGetProcAddress");
    _glfw.wgl.opengl32.MakeCurrent = (WGLMAKECURRENT_T)
        GetProcAddress(_glfw.wgl.opengl32.instance, "wglMakeCurrent");
    _glfw.wgl.opengl32.ShareLists = (WGLSHARELISTS_T)
        GetProcAddress(_glfw.wgl.opengl32.instance, "wglShareLists");

    if (!_glfw.wgl.opengl32.CreateContext ||
        !_glfw.wgl.opengl32.DeleteContext ||
        !_glfw.wgl.opengl32.GetProcAddress ||
        !_glfw.wgl.opengl32.MakeCurrent ||
        !_glfw.wgl.opengl32.ShareLists)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "WGL: Failed to load opengl32 functions");
        return GLFW_FALSE;
    }

    return GLFW_TRUE;
}

// Terminate WGL
//
void _glfwTerminateContextAPI(void)
{
    if (_glfw.wgl.opengl32.instance)
        FreeLibrary(_glfw.wgl.opengl32.instance);

    _glfwDestroyContextTLS();
}

#define setWGLattrib(attribName, attribValue) \
{ \
    attribs[index++] = attribName; \
    attribs[index++] = attribValue; \
    assert((size_t) index < sizeof(attribs) / sizeof(attribs[0])); \
}

// Create the OpenGL or OpenGL ES context
//
int _glfwCreateContext(_GLFWwindow* window,
                       const _GLFWctxconfig* ctxconfig,
                       const _GLFWfbconfig* fbconfig)
{
    int attribs[40];
    int pixelFormat = 0;
    PIXELFORMATDESCRIPTOR pfd;
    HGLRC share = NULL;
    _GLFWcontext* context = window->context;

    if (ctxconfig->share)
        share = ctxconfig->share->wgl.handle;

    context->wgl.dc = GetDC(window->win32.handle);
    if (!context->wgl.dc)
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "WGL: Failed to retrieve DC for window");
        return GLFW_FALSE;
    }

    if (!choosePixelFormat(context, fbconfig, &pixelFormat))
        return GLFW_FALSE;

    if (!DescribePixelFormat(context->wgl.dc, pixelFormat, sizeof(pfd), &pfd))
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "WGL: Failed to retrieve PFD for selected pixel format");
        return GLFW_FALSE;
    }

    if (!SetPixelFormat(context->wgl.dc, pixelFormat, &pfd))
    {
        _glfwInputError(GLFW_PLATFORM_ERROR,
                        "WGL: Failed to set selected pixel format");
        return GLFW_FALSE;
    }

    if (context->wgl.ARB_create_context)
    {
        int index = 0, mask = 0, flags = 0;

        if (ctxconfig->api == GLFW_OPENGL_API)
        {
            if (ctxconfig->forward)
                flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

            if (ctxconfig->profile == GLFW_OPENGL_CORE_PROFILE)
                mask |= WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
            else if (ctxconfig->profile == GLFW_OPENGL_COMPAT_PROFILE)
                mask |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
        }
        else
            mask |= WGL_CONTEXT_ES2_PROFILE_BIT_EXT;

        if (ctxconfig->debug)
            flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

        if (ctxconfig->robustness)
        {
            if (context->wgl.ARB_create_context_robustness)
            {
                if (ctxconfig->robustness == GLFW_NO_RESET_NOTIFICATION)
                {
                    setWGLattrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                                 WGL_NO_RESET_NOTIFICATION_ARB);
                }
                else if (ctxconfig->robustness == GLFW_LOSE_CONTEXT_ON_RESET)
                {
                    setWGLattrib(WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
                                 WGL_LOSE_CONTEXT_ON_RESET_ARB);
                }

                flags |= WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB;
            }
        }

        if (ctxconfig->release)
        {
            if (context->wgl.ARB_context_flush_control)
            {
                if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_NONE)
                {
                    setWGLattrib(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
                                 WGL_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB);
                }
                else if (ctxconfig->release == GLFW_RELEASE_BEHAVIOR_FLUSH)
                {
                    setWGLattrib(WGL_CONTEXT_RELEASE_BEHAVIOR_ARB,
                                 WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB);
                }
            }
        }

        // NOTE: Only request an explicitly versioned context when necessary, as
        //       explicitly requesting version 1.0 does not always return the
        //       highest version supported by the driver
        if (ctxconfig->major != 1 || ctxconfig->minor != 0)
        {
            setWGLattrib(WGL_CONTEXT_MAJOR_VERSION_ARB, ctxconfig->major);
            setWGLattrib(WGL_CONTEXT_MINOR_VERSION_ARB, ctxconfig->minor);
        }

        if (flags)
            setWGLattrib(WGL_CONTEXT_FLAGS_ARB, flags);

        if (mask)
            setWGLattrib(WGL_CONTEXT_PROFILE_MASK_ARB, mask);

        setWGLattrib(0, 0);

        context->wgl.handle = context->wgl.CreateContextAttribsARB(context->wgl.dc,
                                                                   share,
                                                                   attribs);
        if (!context->wgl.handle)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                            "WGL: Failed to create OpenGL context");
            return GLFW_FALSE;
        }
    }
    else
    {
        context->wgl.handle = _glfw_wglCreateContext(context->wgl.dc);
        if (!context->wgl.handle)
        {
            _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                            "WGL: Failed to create OpenGL context");
            return GLFW_FALSE;
        }

        if (share)
        {
            if (!_glfw_wglShareLists(share, context->wgl.handle))
            {
                _glfwInputError(GLFW_PLATFORM_ERROR,
                                "WGL: Failed to enable sharing with specified OpenGL context");
                return GLFW_FALSE;
            }
        }
    }

    _glfwPlatformMakeContextCurrent(window);
    initWGLExtensions(context);

    return GLFW_TRUE;
}

#undef setWGLattrib

// Destroy the OpenGL context
//
void _glfwDestroyContext(_GLFWwindow* window)
{
    _GLFWcontext* context = window->context;

    if (context->wgl.handle)
    {
        _glfw_wglDeleteContext(context->wgl.handle);
        context->wgl.handle = NULL;
    }

    if (context->wgl.dc)
    {
        ReleaseDC(window->win32.handle, context->wgl.dc);
        context->wgl.dc = NULL;
    }
}

// Analyzes the specified context for possible recreation
//
int _glfwAnalyzeContext(const _GLFWcontext* context,
                        const _GLFWctxconfig* ctxconfig,
                        const _GLFWfbconfig* fbconfig)
{
    int required = GLFW_FALSE;

    if (ctxconfig->api == GLFW_OPENGL_API)
    {
        if (ctxconfig->forward)
        {
            if (!context->wgl.ARB_create_context)
            {
                _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                                "WGL: A forward compatible OpenGL context requested but WGL_ARB_create_context is unavailable");
                return _GLFW_RECREATION_IMPOSSIBLE;
            }

            required = GLFW_TRUE;
        }

        if (ctxconfig->profile)
        {
            if (!context->wgl.ARB_create_context_profile)
            {
                _glfwInputError(GLFW_VERSION_UNAVAILABLE,
                                "WGL: OpenGL profile requested but WGL_ARB_create_context_profile is unavailable");
                return _GLFW_RECREATION_IMPOSSIBLE;
            }

            required = GLFW_TRUE;
        }

        if (ctxconfig->release)
        {
            if (context->wgl.ARB_context_flush_control)
                required = GLFW_TRUE;
        }
    }
    else
    {
        if (!context->wgl.ARB_create_context ||
            !context->wgl.ARB_create_context_profile ||
            !context->wgl.EXT_create_context_es2_profile)
        {
            _glfwInputError(GLFW_API_UNAVAILABLE,
                            "WGL: OpenGL ES requested but WGL_ARB_create_context_es2_profile is unavailable");
            return _GLFW_RECREATION_IMPOSSIBLE;
        }

        required = GLFW_TRUE;
    }

    if (ctxconfig->major != 1 || ctxconfig->minor != 0)
    {
        if (context->wgl.ARB_create_context)
            required = GLFW_TRUE;
    }

    if (ctxconfig->debug)
    {
        if (context->wgl.ARB_create_context)
            required = GLFW_TRUE;
    }

    if (fbconfig->samples > 0)
    {
        // MSAA is not a hard constraint, so do nothing if it's not supported
        if (context->wgl.ARB_multisample && context->wgl.ARB_pixel_format)
            required = GLFW_TRUE;
    }

    if (fbconfig->sRGB)
    {
        // sRGB is not a hard constraint, so do nothing if it's not supported
        if ((context->wgl.ARB_framebuffer_sRGB ||
             context->wgl.EXT_framebuffer_sRGB) &&
            context->wgl.ARB_pixel_format)
        {
            required = GLFW_TRUE;
        }
    }

    if (required)
        return _GLFW_RECREATION_REQUIRED;

    return _GLFW_RECREATION_NOT_NEEDED;
}


//////////////////////////////////////////////////////////////////////////
//////                       GLFW platform API                      //////
//////////////////////////////////////////////////////////////////////////

void _glfwPlatformMakeContextCurrent(_GLFWwindow* window)
{
    if (window)
    {
        _glfw_wglMakeCurrent(window->context->wgl.dc,
                             window->context->wgl.handle);
    }
    else
        _glfw_wglMakeCurrent(NULL, NULL);

    _glfwSetContextTLS(window);
}

void _glfwPlatformSwapBuffers(_GLFWwindow* window)
{
    _GLFWcontext* context = window->context;

    // HACK: Use DwmFlush when desktop composition is enabled
    if (_glfwIsCompositionEnabled() && !window->monitor)
    {
        int count = context->wgl.interval;
        while (count--)
            _glfw_DwmFlush();
    }

    SwapBuffers(context->wgl.dc);
}

void _glfwPlatformSwapInterval(int interval)
{
    _GLFWwindow* window = _glfwPlatformGetCurrentContext();
    _GLFWcontext* context = window->context;

    context->wgl.interval = interval;

    // HACK: Disable WGL swap interval when desktop composition is enabled to
    //       avoid interfering with DWM vsync
    if (_glfwIsCompositionEnabled() && !window->monitor)
        interval = 0;

    if (context->wgl.EXT_swap_control)
        context->wgl.SwapIntervalEXT(interval);
}

int _glfwPlatformExtensionSupported(const char* extension)
{
    const char* extensions;
    _GLFWwindow* window = _glfwPlatformGetCurrentContext();
    _GLFWcontext* context = window->context;

    if (context->wgl.GetExtensionsStringEXT != NULL)
    {
        extensions = context->wgl.GetExtensionsStringEXT();
        if (extensions)
        {
            if (_glfwStringInExtensionString(extension, extensions))
                return GLFW_TRUE;
        }
    }

    if (context->wgl.GetExtensionsStringARB != NULL)
    {
        extensions = context->wgl.GetExtensionsStringARB(context->wgl.dc);
        if (extensions)
        {
            if (_glfwStringInExtensionString(extension, extensions))
                return GLFW_TRUE;
        }
    }

    return GLFW_FALSE;
}

GLFWglproc _glfwPlatformGetProcAddress(const char* procname)
{
    const GLFWglproc proc = (GLFWglproc) _glfw_wglGetProcAddress(procname);
    if (proc)
        return proc;

    return (GLFWglproc) GetProcAddress(_glfw.wgl.opengl32.instance, procname);
}


//////////////////////////////////////////////////////////////////////////
//////                        GLFW native API                       //////
//////////////////////////////////////////////////////////////////////////

GLFWAPI HGLRC glfwGetWGLContext(GLFWwindow* handle)
{
    _GLFWwindow* window = (_GLFWwindow*) handle;

    _GLFW_REQUIRE_INIT_OR_RETURN(NULL);

    if (!window->context)
    {
        _glfwInputError(GLFW_NO_CONTEXT, NULL);
        return NULL;
    }

    return window->context->wgl.handle;
}

