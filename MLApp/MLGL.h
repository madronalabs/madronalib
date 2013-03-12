
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_GL__
#define __ML_GL__

#include "MLVector.h"

// TODO make GLView

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

void orthoView(int width, int height);

#if ML_MAC // TODO
void worldView(float aspect);
#endif

const int kGLViewNColors = 8;
const float kGLViewIndicatorColors[kGLViewNColors*4] = 
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

void fillRect(const MLRect& r);
void frameRect(const MLRect& r);





#endif // __ML_GL__