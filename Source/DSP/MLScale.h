
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SCALE_H
#define _ML_SCALE_H

#include <vector>
#include <iomanip>
#include <sstream>
#include "MLDSP.h"
#include "MLDebug.h"

#include "MLDefaultFileLocations.h" // to remove

const int kMLNumRatios = 256;
const int kMLNumScaleNotes = 128;

class MLScale
{

public:
	MLScale();
	~MLScale();

	void operator= (const MLScale& b);
	void setDefaults();

	// load a scale from an input string along with an optional mapping.
	void loadFromString(const std::string& scaleStr, const std::string& mapStr = "");
	
	void loadFromRelativePath(const std::string& path);
	
	// convert a note number into a pitch ratio from 440.0 Hz, using the currently loaded scale.
	float noteToPitch(float note) const;
	float noteToPitch(int note) const;

	// return pitch of the given note in log pitch (1.0 per octave) space with 440.0Hz = 0.
	float noteToLogPitch(float note) const;

	// return log pitch of the note of the current scale closest to the input.
	float quantizePitch(float a) const;
	
	void setName(const std::string& nameStr);
	void setDescription(const std::string& descStr);
	void dump();

private:
	void clear();
	
	// add a ratio expressed as a rational number.
	void addRatio(int n, int d);
	
	// add a ratio expressed in cents.
	void addRatio(double c);
	
	// recalculate of all ratios in mRatios.
	void recalcRatios();

	// load a map from an input string.
	// returns the number of notes in the map.
	int loadMappingFromString(const std::string& mapStr);
	void setDefaultMapping();

	void setDefaultScale();

	struct keyMap
	{
		int mMapSize;
		int mNoteStart;
		int mNoteEnd;
		
		// Middle note where the first entry of the mapping is placed
		int mFirstMapNote;
		
		// note that points to the ratio 1/1
		int mTonicNote;
		
		// absolute frequency of tonic
		float mTonicFreq;
		
		// Scale degree to consider as formal octave
		int mOctaveScaleDegree;
		
		std::vector<int> mNotes;
	};

	keyMap mKeyMap;

	std::string mName;
	std::string mDescription;
			
	// list of ratios forming a scale.  The first entry is always 1.0, or 0 cents. 
	// For scales that repeat on 2/1 octaves the last entry will always be 2.  
	std::vector<double> mRatioList;
	
	// all possible pitches stored as ratios of pitch to the tonic frequency.
	float mRatios[kMLNumRatios];	
	
	// pitches stored in linear octave space. pitch = log2(ratio).
	float mPitches[kMLNumRatios];
	
	std::string mScalePath; 
};


#endif // _ML_SCALE_H