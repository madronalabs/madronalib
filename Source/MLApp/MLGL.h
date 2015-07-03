
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

// NOTE: this is the old MLGL based on GLUT and immediate mode. Deprecated!

#ifndef _MLGL_
#define _MLGL_

#include "MLVector.h"
#include "MLPlatform.h"


#undef JUCE_OPENGL
#define JUCE_OPENGL 1

#if ML_IOS
#define JUCE_OPENGL_ES 1
#endif

// Windows
#if ML_WINDOWS
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
// Linux
#elif ML_LINUX
	#include <GL/gl.h>
	#undef KeyPress
// IOS
#elif ML_IOS
	#if defined (__IPHONE_7_0) && __IPHONE_OS_VERSION_MAX_ALLOWED >= __IPHONE_7_0
		#include <OpenGLES/ES3/gl.h>
	#else
		#include <OpenGLES/ES2/gl.h>
	#endif
// Mac OS
#elif ML_MAC
/*
	#if defined (MAC_OS_X_VERSION_10_7) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_7)
		#define JUCE_OPENGL3 1
		#include <OpenGL/gl3.h>
		#include <OpenGL/gl3ext.h>
	#else
 */
		#include <OpenGL/gl.h>
		#include <OpenGL/glext.h>

#endif



// OpenGL helper utilities. 
// This is all considered temporary, because it is all in immediate mode, which is a bad way to write 
// OpenGL code. TODO consider finding a good 2d GL library written by someone else.


#if ML_WINDOWS
 #include <Windows.h>
 #include <gl/gl.h>
 #include <gl/glu.h>
#elif ML_LINUX
 #include <GL/gl.h>
 #include <GL/glut.h>
 #undef KeyPress
#elif ML_IOS
 #include <OpenGLES/ES1/gl.h>
 #include <OpenGLES/ES1/glext.h>
#elif ML_MAC
 #include <GLUT/glut.h>
#endif



namespace MLGLData
{
    static const int indicatorColorsSize = 1 << 3;
    static const int indicatorColorsMask = indicatorColorsSize - 1;
    const float indicatorColors[indicatorColorsSize*4] =
    {
        0.0705, 0.501, 0.670, 1.,
        0.352, 0.4, 0.647, 1.,
        0.623, 0.341, 0.623, 1.,
        0.701, 0.286, 0.243, 1.,
        0.788, 0.435, 0.223, 1.,
        0.784, 0.592, 0.278, 1.,
        0.737, 0.647, 0.494, 1.,
        0.415, 0.454, 0.231, 1.
    };
};

class MLGL
{
public:
     
    static void orthoView(int width, int height);
    static void orthoView2(int width, int height);
    void orthoViewForAspectInBounds(const float a, const MLRect bounds);
    
#if ML_MAC // TODO
    static void worldView(float aspect);
#endif
   
    static const float* getIndicatorColor(int i) { return MLGLData::indicatorColors + 4*(i & MLGLData::indicatorColorsMask); }
    static void fillRect(const MLRect& r);
    static void strokeRect(const MLRect& r, float viewScale);
    
    static void drawTextAt(float x, float y, float z, float textScale, float viewScale, const char* ps);
    
    static Vec2 worldToScreen(const Vec3& world);

    static void drawDot(Vec2 pos, float r); // TEMP

};




#endif // _MLGL_