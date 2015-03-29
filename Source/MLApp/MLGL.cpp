
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLVector.h"
#include "MLGL.h"

void MLGL::orthoView(int width, int height)
{
    glViewport(0,0,width,height);    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();    
    glOrtho(0.0f,width,height,0.0f,-1.0f,1.0f);    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void MLGL::orthoView2(int width, int height)
{
    glViewport(0,0,width,height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0f, width, 0.0f, height, -1.0f, 1.0f);    
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

#if ML_MAC // TEMP add GLUT Windows
void MLGL::worldView(float aspect)
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

// TEMP
void MLGL::fillRect(const MLRect& r)
{
	glBegin(GL_QUADS);
	const MLRect tr = r;
	glVertex2f(tr.left(), tr.top());
	glVertex2f(tr.right(), tr.top());
	glVertex2f(tr.right(), tr.bottom());
	glVertex2f(tr.left(), tr.bottom());
	glEnd();
}

// TEMP
void MLGL::strokeRect(const MLRect& r, float viewScale)
{	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glDisable(GL_LINE_SMOOTH);
	glLineWidth(1.0*viewScale);
	
	glBegin(GL_LINE_LOOP);
//	const MLRect tr = r.translated(Vec2(0.5f, 0.5f));
	glVertex2f(r.left(), r.top());
	glVertex2f(r.right(), r.top());
	glVertex2f(r.right(), r.bottom());
	glVertex2f(r.left(), r.bottom());
	glEnd();
}

// TEMP
void MLGL::drawTextAt(float x, float y, float z, float textScale, float viewScale, const char* ps)
{
#ifdef ML_MAC
	int len, i;
	glPushMatrix();
	glTranslatef(x, y, z);
	glScalef(textScale*viewScale, textScale*viewScale, z);
	len = (int) strlen(ps);
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(1.0*viewScale);
	
	for (i = 0; i < len; i++)
	{
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, ps[i]);
	}
	glLineWidth(1.0f);
	glPopMatrix();
	
	
#endif
}

Vec2 MLGL::worldToScreen(const Vec3& world)
{
	GLint viewport[4];
	GLdouble mvmatrix[16], projmatrix[16];
	GLdouble wx, wy, wz;
    
	glGetIntegerv(GL_VIEWPORT, viewport);
	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);
	
	GLint result = gluProject(world[0], world[1], world[2],
                              mvmatrix, projmatrix, viewport,
                              &wx, &wy, &wz);	
	
	if (result == GL_TRUE)
	{
		return Vec2(wx, wy);
	}
	else
	{
		return Vec2(0, 0);
	}
}

// temporary, ugly
void MLGL::drawDot(Vec2 pos, float r)
{    
	int steps = 16;
    
	float x = pos.x();
	float y = pos.y();
    
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(x, y);
	for(int i=0; i<=steps; ++i)
	{
		float theta = kMLTwoPi * (float)i / (float)steps;
		float rx = r*cosf(theta);
		float ry = r*sinf(theta);
		glVertex3f(x + rx, y + ry, 0.f);
	}
	glEnd();
}	
