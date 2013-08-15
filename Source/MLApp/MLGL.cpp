
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLVector.h"
#include "MLGL.h"

void orthoView(int width, int height)
{
    glViewport(0,0,width,height);
 
    glMatrixMode(GL_PROJECTION);  
    glLoadIdentity();
 
    glOrtho(0.0f,width,height,0.0f,-1.0f,1.0f);
 
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

#if ML_MAC // TEMP add GLUT Windows
void worldView(float aspect)
{
 	// setup world viewing coordinates
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(8.0, aspect, 0.5, 50.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0, -15.0, 5.0, // eyepoint x y z
			  0.0, 0.0, -0.25, // center x y z
			  0.0, 1.0, 0.0); // up vector
}
#endif

void fillRect(const MLRect& r)
{
	glBegin(GL_QUADS);
	const MLRect tr = r;
	glVertex2f(tr.left(), tr.top());
	glVertex2f(tr.right(), tr.top());
	glVertex2f(tr.right(), tr.bottom());
	glVertex2f(tr.left(), tr.bottom());
	glEnd();
}

void frameRect(const MLRect& r)
{
	glBegin(GL_LINE_LOOP);
	const MLRect tr = r.translated(Vec2(0.5f, 0.5f));
	glVertex2f(tr.left(), tr.top());
	glVertex2f(tr.right()-1, tr.top());
	glVertex2f(tr.right()-1, tr.bottom()-1);
	glVertex2f(tr.left(), tr.bottom()-1);
	glEnd();
}
