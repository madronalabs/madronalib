
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SCALE_H
#define _ML_SCALE_H

#include <vector>
#include <iomanip>
#include <sstream>
#include "MLDSPDeprecated.h"
#include "MLDebug.h"
#include "MLText.h"

#include "MLFile.h" // TODO shouldn't know about files here
//#include "MLDefaultFileLocations.h" // to remove

const int kMLNumRatios = 256;
const int kMLUnmappedNote = kMLNumRatios + 1;

class MLScale
{
public:
	// a path for all class members to treat as root.
	// TDO migrate to ml::Text
	static void setRootPath(String root);
	static String mRootPath;

	MLScale();
	~MLScale();

	void operator= (const MLScale& b);
	void setDefaults();

	// load a scale from an input string along with an optional mapping.
	void loadFromString(const std::string& scaleStr, const std::string& mapStr = "");
	
	void loadFromRelativePath(ml::Text path);
	
	// convert a note number into a pitch ratio from 440.0 Hz, using the currently loaded scale.
	float noteToPitch(float note) const;
	float noteToPitch(int note) const;

	// return pitch of the given note in log pitch (1.0 per octave) space with 440.0Hz = 0.
	float noteToLogPitch(float note) const;

	// return log pitch of the note of the current scale closest to the input.
	float quantizePitch(float a) const;
	float quantizePitchNearest(float a) const;
	
	void setName(const std::string& nameStr);
	void setDescription(const std::string& descStr);
	void dump();

private:
	void clear();
	void addRatioAsFraction(int n, int d);
	void addRatioAsCents(double c);
	double middleNoteRatio(int n);
	void recalcRatios();
	int loadMappingFromString(const std::string& mapStr);
	void setDefaultMapping();
	void setDefaultScale();
	
	// key map structure and utility functions
	static const int kMaxMappingSize = 128;
	struct keyMap
	{
		int mMapSize;
		int mNoteStart;
		int mNoteEnd;
		
		// Middle note where the first entry of the mapping is placed
		int mMiddleNote;
		
		// note that is defined to be the reference frequency
		int mReferenceNote;
		
		// reference frequency
		float mReferenceFreq;
		
		// Scale degree to consider as formal octave
		int mOctaveScaleDegree;
		
		std::array<int, kMaxMappingSize> mNoteIndexes;
	};
	
	inline void clearKeyMap(keyMap& map)
	{
		map.mNoteIndexes.fill(-1);
	}
	
	inline void addIndexToKeyMap(keyMap& map, int newIdx)
	{
		// overwrite last element, or fail
		for(auto& idx : map.mNoteIndexes)
		{
			if(idx == -1)
			{
				idx = newIdx;
				break;
			}
		}
	}

	keyMap mKeyMap;

	std::string mName;
	std::string mDescription;
			
	// list of ratios forming a scale.  The first entry is always 1.0, or 0 cents. 
	// For scales that repeat on 2/1 octaves the last entry will always be 2.  
	std::array<double, kMLNumRatios> mScaleRatios;
	
	inline void addRatio(double newRatio)
	{
		// overwrite last element, or fail
		for(auto& r : mScaleRatios)
		{
			if(r == 0.)
			{
				r = newRatio;
				break;
			}
		}
	}
	
	inline int countRatios()
	{
		int n = 0;
		for(auto& r : mScaleRatios)
		{
			if(r == 0.)
			{
				break;
			}
			else
			{
				n++;
			}
		}
		return n;
	}
	
	std::array<bool, kMLNumRatios> mNoteIsMapped;	
	
	// all possible pitches stored as ratios of pitch to 440.0 Hz.
	std::array<double, kMLNumRatios> mRatios;	
	
	// pitches stored in linear octave space. pitch = log2(ratio).
	std::array<double, kMLNumRatios> mPitches;	
	
	// the currently loaded scale's path relative to the the root set by setRootPath().
	static ml::Text mRelativePath;

};


#endif // _ML_SCALE_H
