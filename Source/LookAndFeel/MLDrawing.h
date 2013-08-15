
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_DRAWING_H__
#define __ML_DRAWING_H__

#include "MLDSP.h"
#include "MLUI.h"
#include "MLLookAndFeel.h"

class MLDrawing :
	public Component,
	public MLWidget
{
public:
	typedef enum
	{
		drawLine,
		drawLineArrowStart,
		drawLineArrowEnd,
		drawLineArrowBoth,
		setLineThickness,
		setLightColor,
		setDarkColor,
	}	eOperationType;
	
	class Operation
	{
	public:
		Operation(eOperationType, float, float, float, float);
		Operation(eOperationType, int, int, int, int);
		~Operation();
		
		inline int argAsInt(int i);

		eOperationType type;
		float args [4];
	};
	
	MLDrawing();
	~MLDrawing();
	
	// add point and return point index
	int addPoint(const Vec2& p);
	void addOperation(eOperationType op);
	void addOperation(eOperationType op, float arg1, float arg2 = 0., float arg3 = 0., float arg4 = 0.);
	void addOperation(eOperationType op, int arg1, int arg2 = 0, int arg3 = 0, int arg4 = 0);
	void dumpOperations();
	void paint(Graphics& g);

	void setPixelOffset(const Vec2& f);	

	void resizeWidget(const MLRect& b, const int);
	
private:
	void drawArrowhead(Graphics& g, const Vec2& p1, const Vec2& p2, float scale);

	Vec2 mPixelOffset;
	float mLineThickness;
	Colour mLineColor;
	Colour mLightColor;
	Colour mDarkColor;
	std::vector<Vec2> mGridPoints;
	std::vector<Vec2> mTransformedPoints;
	std::vector<Operation> mOperations;
	
//	inline Vec2 adjust (Vec2 p) { return Vec2(floor(p[0] + 0.5f, floor(p[1] + 0.5f ); }
};


#endif // __ML_DRAWING_H__