
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef _ML_SCALE_H
#define _ML_SCALE_H

#include <vector>

#include "MLDSP.h"
#include "MLDebug.h"

const int kMLNumRatios = 256;
const int kMLNumScaleNotes = 128;

class MLScale
{

public:
	MLScale();
	~MLScale();

	void operator= (const MLScale& b);
	void setDefaultScale();
	void clear();
	
	// add a ratio expressed as a rational number.
	void addRatio(unsigned n, unsigned d);
	
	// add a ratio expressed in cents.
	void addRatio(double c);

	// recalculate of all ratios in mRatios.
	void recalcRatios();
	
	void setDefaultMapping();
	
	// convert a note number into a pitch ratio using the currently loaded scale. 
	// if the scale has been changed since its last use, recalculate.
	float noteToPitch(float note);
	float noteToPitch(int note);

	float quantizePitch(float a);
	
//	double noteToFrequency(unsigned note);  // requires tonic, maybe implement with .kbm mappings

	void setName(const char* nameStr);
	void setDescription(const char* descStr);
	void dump();
	
private:
	
	std::string mName;
	std::string mDescription;
	
	// mNotes entry that points to the ratio 1/1
	unsigned mTonicNote;
	
	// list of ratios forming a scale.  The first entry is always 1.0, or 0 cents. 
	// For scales that repeat on octaves the last entry will always be 2.  
	std::vector<double> mRatioList;
	
	// all possible pitches stored as exponents e where tonic * 2^e = frequency.
	float mRatios[kMLNumRatios];	
	
	// pitches stored in linear octave space. pitch = log2(ratio).
	float mPitches[kMLNumRatios];	
	
	// mappings from note number to pitch number.
	unsigned mNotes[kMLNumScaleNotes];
	
};


#endif // _ML_SCALE_H