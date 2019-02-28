
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SCALE_H
#define _ML_SCALE_H

#include <vector>
#include <iomanip>
#include <sstream>
#include <array>
#include <cmath>

#include "MLScalarMath.h"

//#include "MLText.h"

#include "MLFile.h" // TODO shouldn't know about files here

// number of notes to calculate. While only 1-127 are needed for MIDI notes,
// the higher ones are used in Virta to quantize partials.
const int kMLNumNotes = 256;

const int kMLUnmappedNote = kMLNumNotes + 1;

class MLScale
{
public:
	// a path for all class members to treat as root.
	// TODO migrate to ml::Text
	static void setRootPath(String root);
	static String mRootPath;

	MLScale();
	~MLScale();

	void operator= (const MLScale& b);
	void setDefaults();

	// load a scale from an input string along with an optional mapping.
	void loadScaleFromString(const std::string& scaleStr, const std::string& mapStr = "");
	
	void loadFromRelativePath(ml::Text path);
	
	// return pitch of the given note in log pitch (1.0 per octave) space with 440.0Hz = 0.
	float noteToLogPitch(float note) const;

	// return log pitch of the note of the current scale closest to the input.
	float quantizePitch(float a) const;
	float quantizePitchNearest(float a) const;
	
	void setName(const std::string& nameStr);
	void setDescription(const std::string& descStr);
//	void dump();

private:

	float noteToPitch(float note) const;

	void clear();
	void addRatioAsFraction(int n, int d);
	void addRatioAsCents(double c);
	double middleNoteRatio(int n);
	void recalcRatiosAndPitches();
	int loadMappingFromString(const std::string& mapStr);
	void setDefaultMapping();
	void setDefaultScale();
	
	// key map structure and utility functions

	struct keyMap
	{
		int mSize;
		
		// Middle note where the first entry of the mapping is placed
		int mMiddleNote;
		
		// note that is defined to be the reference frequency
		int mReferenceNote;
		
		// reference frequency
		float mReferenceFreq;
		
		// Scale degree to consider as formal octave
		int mOctaveScaleDegree;
		
		// scale degree for each note
		std::array<int, kMLNumNotes> mNoteDegrees;
	};
	
	inline void clearKeyMap(keyMap& map)
	{
		map.mNoteDegrees.fill(-1);
		map.mSize = 0;
	}
	
	inline void addNoteToKeyMap(keyMap& map, int newIdx)
	{
		if(map.mSize < kMLNumNotes)
		{
			map.mNoteDegrees[map.mSize++] = newIdx;
		}
	}

	keyMap mKeyMap;

	std::string mName;
	std::string mDescription;

	// list of ratios forming a scale.  The first entry is always 1.0, or 0 cents.
	// The last entry is the ratio of an octave, typically but not always 2.

	// TODO use Ratios
	std::array<double, kMLNumNotes> mScaleRatios;
	
	int mScaleSize;
	inline void addRatio(double newRatio)
	{
		if(mScaleSize < kMLNumNotes)
		{
			mScaleRatios[mScaleSize++] = newRatio;
		}
	}
	
	// pitch for each integer note number stored in as a ratio p/k where k = 440.0 Hz
	std::array<double, kMLNumNotes> mRatios;
	
	// pitch for each integer note number stored in linear octave space. pitch = log2(ratio).
	std::array<double, kMLNumNotes> mPitches;

};


#endif // _ML_SCALE_H
