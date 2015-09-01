//
//  MLJuceOpenGL.h
//  Soundplane
//
//  Created by Randy Jones on 3/28/15.
//
//


/*
 ==============================================================================
 
 This file is part of the JUCE library.
 Copyright (c) 2013 - Raw Material Software Ltd.
 
 Permission is granted to use this software under the terms of either:
 a) the GPL v2 (or any later version)
 b) the Affero GPL v3
 
 Details of these licenses can be found at: www.gnu.org/licenses
 
 JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 
 ------------------------------------------------------------------------------
 
 To release a closed-source product which uses JUCE, commercial licenses are
 available: visit www.juce.com for more information.
 
 ==============================================================================
 */

#ifndef JUCE_OPENGL_H_INCLUDED
#define JUCE_OPENGL_H_INCLUDED

#include "modules/juce_gui_extra/juce_gui_extra.h"

#undef JUCE_OPENGL
#define JUCE_OPENGL 1

#if JUCE_IOS || JUCE_ANDROID
#define JUCE_OPENGL_ES 1
#endif

#if ! JUCE_ANDROID
#define JUCE_OPENGL_CREATE_JUCE_RENDER_THREAD 1
#endif

#if JUCE_WINDOWS
#ifndef APIENTRY
#define APIENTRY __stdcall
#define CLEAR_TEMP_APIENTRY 1
#endif
#ifndef WINGDIAPI
#define WINGDIAPI __declspec(dllimport)
#define CLEAR_TEMP_WINGDIAPI 1
#endif
#include <gl/GL.h>
#ifdef CLEAR_TEMP_WINGDIAPI
#undef WINGDIAPI
#undef CLEAR_TEMP_WINGDIAPI
#endif
#ifdef CLEAR_TEMP_APIENTRY
#undef APIENTRY
#undef CLEAR_TEMP_APIENTRY
#endif
#elif JUCE_LINUX
#include <GL/gl.h>
#undef KeyPress
#elif JUCE_IOS
#if defined (__IPHONE_7_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_7_0
#include <OpenGLES/ES3/gl.h>
#else
#include <OpenGLES/ES2/gl.h>
#endif
#elif JUCE_MAC
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif JUCE_ANDROID
#include <GLES2/gl2.h>
#endif


//=============================================================================
#if JUCE_OPENGL_ES || defined (DOXYGEN)
/** This macro is a helper for use in GLSL shader code which needs to compile on both GLES and desktop GL.
 Since it's mandatory in GLES to mark a variable with a precision, but the keywords don't exist in normal GLSL,
 these macros define the various precision keywords only on GLES.
 */
#define JUCE_MEDIUMP  "mediump"

/** This macro is a helper for use in GLSL shader code which needs to compile on both GLES and desktop GL.
 Since it's mandatory in GLES to mark a variable with a precision, but the keywords don't exist in normal GLSL,
 these macros define the various precision keywords only on GLES.
 */
#define JUCE_HIGHP    "highp"

/** This macro is a helper for use in GLSL shader code which needs to compile on both GLES and desktop GL.
 Since it's mandatory in GLES to mark a variable with a precision, but the keywords don't exist in normal GLSL,
 these macros define the various precision keywords only on GLES.
 */
#define JUCE_LOWP     "lowp"
#else
#define JUCE_MEDIUMP
#define JUCE_HIGHP
#define JUCE_LOWP
#endif

//=============================================================================
namespace juce
{
	
	class OpenGLTexture;
	class OpenGLFrameBuffer;
	class OpenGLShaderProgram;

#include "modules/juce_opengl/geometry/juce_Quaternion.h"
#include "modules/juce_opengl/geometry/juce_Matrix3D.h"
#include "modules/juce_opengl/geometry/juce_Vector3D.h"
#include "modules/juce_opengl/geometry/juce_Draggable3DOrientation.h"
#include "modules/juce_opengl/native/juce_MissingGLDefinitions.h"
#include "modules/juce_opengl/opengl/juce_OpenGLHelpers.h"
#include "modules/juce_opengl/opengl/juce_OpenGLPixelFormat.h"
#include "modules/juce_opengl/native/juce_OpenGLExtensions.h"
#include "modules/juce_opengl/opengl/juce_OpenGLRenderer.h"
#include "modules/juce_opengl/opengl/juce_OpenGLContext.h"
#include "modules/juce_opengl/opengl/juce_OpenGLFrameBuffer.h"
#include "modules/juce_opengl/opengl/juce_OpenGLGraphicsContext.h"
#include "modules/juce_opengl/opengl/juce_OpenGLHelpers.h"
#include "modules/juce_opengl/opengl/juce_OpenGLImage.h"
#include "modules/juce_opengl/opengl/juce_OpenGLRenderer.h"
#include "modules/juce_opengl/opengl/juce_OpenGLShaderProgram.h"
#include "modules/juce_opengl/opengl/juce_OpenGLTexture.h"
#include "modules/juce_opengl/utils/juce_OpenGLAppComponent.h"

}

#endif   // JUCE_OPENGL_H_INCLUDED
