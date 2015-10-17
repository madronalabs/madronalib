
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLUI.h"

MLPoint getPixelCenter(const MLRect& r) 
{
	int w = r[2];
	int h = r[3];
	w += w & 1;
	h += h & 1;
	return Vec2(w/2, h/2);
}


#pragma mark color utilities

const juce::Colour createMLBaseColour (const juce::Colour& buttonColour,
                                      const bool hasKeyboardFocus,
                                      const bool,
                                      const bool) throw()
{
    const float sat = hasKeyboardFocus ? 1.3f : 0.9f;
    const juce::Colour baseColour (buttonColour.withMultipliedSaturation (sat));
    return baseColour;
}

// return brighter, less saturated vesion of thumb-- for drawing lines etc.
const juce::Colour brightLineColor (const juce::Colour& c)
{
	float sat = min(c.getSaturation(), 0.60f);
	float val = 1.f;
	return juce::Colour(c.getHue(), sat, val, 1.f);
}

const juce::Colour brightColor (const juce::Colour& c)
{
	float g = c.getFloatGreen();
	float val = min((c.getBrightness() + (1.f - g)*0.5f), 1.f);
	return c.withBrightness(val);
}

const juce::Colour brighterColor (const juce::Colour& c)
{
	float g = c.getFloatGreen();
	float val = min((c.getBrightness() + (1.f - g)*1.f), 1.f);
	return c.withBrightness(val);
}

const juce::Colour darkColor (const juce::Colour& c)
{
	return c.overlaidWith(juce::Colours::darkgrey.withAlpha(0.15f));
}

const juce::Colour darkerColor (const juce::Colour& c)
{
	return c.overlaidWith(juce::Colours::black.withAlpha(0.25f));
}
