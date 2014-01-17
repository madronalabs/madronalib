
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef __ML_UI__
#define __ML_UI__

#include "JuceHeader.h"

using namespace juce;

#include "MLVector.h"

#ifdef _WIN32
	#include <memory>
#else
	#include <tr1/memory>
#endif

const int kMLNumChars = 19;
const int kMLMaxNumberDigits = 16;

enum MLValueDisplayMode
{
	eMLNumFloat = 0,
	eMLNumZeroIsOff,
	eMLNumSeconds,
	eMLNumHertz,
	eMLNumPitch,
	eMLNumDecibels,
	eMLNumPan,
	eMLNumRatio
};

// styles for rendered text
enum MLTextStyle
{
	eMLPlain = 0,
	eMLItalic = 1,
	eMLTitle = 2,
	eMLCaption = 3,
	eMLCaptionSmall = 4,
	eMLNotice = 5,
	eMLNumStyles = 6
};

enum MLAdornFlags
{
	eMLAdornNone =	0,
	eMLAdornLeft =		1 << 0,
	eMLAdornTopLeft =		1 << 1,
	eMLAdornTop =			1 << 2,
	eMLAdornTopRight =	1 << 3,
	eMLAdornRight =		1 << 4,
	eMLAdornBottomRight = 1 << 5,
	eMLAdornBottom =		1 << 6,
	eMLAdornBottomLeft =	1 << 7,	
	eMLAdornShadow =		1 << 8,
	eMLAdornPressed	=	1 << 9,
	eMLAdornGlow	=	1 << 10,
	eMLAdornFlat	=	1 << 11,
	eMLAdornSplitLeft	=	1 << 12,
	eMLAdornSplitRight	=	1 << 13
};

const float kMLLabelHeight = 6.f;
const float kMLCornerSize = 2.5f;
const float kMLButtonGradMax = 6.f;
const float kMLButtonOutlineThickness = 0.5f;
const float kMLDialMargin = 18.f;
const float kMLTrackThickness = 6.f;
const float kMLShadowThickness = 3.f;
const float kMLShadowOpacity = 0.25f;

const unsigned kMLSignalViewBufferSize = 128;

inline juce::Rectangle<float> MLToJuceRect(const MLRect& b) { return juce::Rectangle<float>(b.left(), b.top(), b.width(), b.height()); }
inline juce::Rectangle<int> MLToJuceRectInt(const MLRect& b) { return juce::Rectangle<int>(b.left(), b.top(), b.width(), b.height()); }
inline Point<float> MLToJucePoint(const Vec2& b) { return Point<float>(b.x(), b.y()); }
	
inline MLRect juceToMLRect(const juce::Rectangle<int>& b) { return MLRect(b.getX(), b.getY(), b.getWidth(), b.getHeight()); }
inline MLRect juceToMLRect(const juce::Rectangle<float>& b) { return MLRect(b.getX(), b.getY(), b.getWidth(), b.getHeight()); }
inline MLPoint juceToMLPoint(const Point<int>& b) { return MLPoint(b.getX(), b.getY()); }
inline MLPoint juceToMLPoint(const Point<float>& b) { return MLPoint(b.getX(), b.getY()); }

MLPoint getPixelCenter(const MLRect& r);

inline MLPoint floatPointToInt(Point<float> fp)
{
	return(MLPoint(fp.getX(), fp.getY()));
}

inline MLPoint correctPoint(MLPoint p)
{
	return(MLPoint(floor(p.x()) + 0.5f, floor(p.y()) + 0.5f));
}

inline MLRect correctRect(const MLRect& r)
{
	return(MLRect(floor(r.left()) + 0.5f, floor(r.top()) + 0.5f, r.width(), r.height()));
}

inline bool approxEqual(const float a, const float b) { return(fabs(b - a) < 0.0001f); }


// --------------------------------------------------------------------------------
#pragma mark color utilities

const juce::Colour createMLBaseColour (const juce::Colour& buttonColour,
                                      const bool hasKeyboardFocus,
                                      const bool,
                                      const bool) throw();
const juce::Colour brightLineColor (const juce::Colour& c);
const juce::Colour brightColor (const juce::Colour& c);
const juce::Colour brighterColor (const juce::Colour& c);
const juce::Colour darkColor (const juce::Colour& c);
const juce::Colour darkerColor (const juce::Colour& c);

// --------------------------------------------------------------------------------
#pragma mark string utilities


const std::string getShortName(const std::string& str);
const std::string stripExtension(const std::string& str);
const std::string getPath(const std::string& str);

#endif // __ML_UI__