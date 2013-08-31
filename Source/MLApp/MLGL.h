
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _MLGL_
#define _MLGL_

#include "MLVector.h"
#include "MLPlatform.h"

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
        0.0, 0.5, 0.7, 1.,
        0.5, 0.2, 0.9, 1.,
        0.8, 0.0, 0.8, 1.,
        0.8, 0.1, 0.4, 1.,
        0.9, 0.3, 0.0, 1.,
        1.0, 0.4, 0.0, 1.,
        0.8, 0.7, 0.0, 1.,
        0.4, 0.7, 0.1, 1.,
    };
};

class MLGL
{
public:
     
    static void orthoView(int width, int height);
    
#if ML_MAC // TODO
    static void worldView(float aspect);
#endif
   
    static const float* getIndicatorColor(int i) { return MLGLData::indicatorColors + 4*(i & MLGLData::indicatorColorsMask); }
    static void fillRect(const MLRect& r);
    static void strokeRect(const MLRect& r);
    
    static void drawTextAt(float x, float y, float z, const char* ps);
    
    static Vec2 worldToScreen(const Vec3& world);

    static void drawDot(Vec2 pos);

};




#endif // _MLGL_