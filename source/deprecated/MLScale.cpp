
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLScale.h"


String MLScale::mRootPath;
void MLScale::setRootPath(String root)
{
	mRootPath = root;
}


MLScale::MLScale()
{
	setDefaults();
}

MLScale::~MLScale()
{

}

void MLScale::operator= (const MLScale& b)
{
	//mName = b.mName;
	//mDescription = b.mDescription;
	mScaleRatios = b.mScaleRatios;
	mPitches = b.mPitches;
}

void MLScale::setDefaults()
{
	setDefaultScale();
	setDefaultMapping();
	recalcRatiosAndPitches();
}

void MLScale::setDefaultScale()
{
	clear();
	setName("12-equal");
	setDescription("The chromatic equal-tempered scale.");
	// make 12-ET scale	
	for(int i=1; i<=12; ++i)
	{
		addRatioAsCents(100.0 * i);
	}
}

void MLScale::setDefaultMapping()
{
	clearKeyMap(mKeyMap);
	
	mKeyMap.mMiddleNote = 69;	// arbitrary in equal-tempered scale
	mKeyMap.mReferenceNote = 69; // A3
	mKeyMap.mReferenceFreq = 440.0f;
	mKeyMap.mOctaveScaleDegree = mScaleSize - 1;

	for(int i = 0; i < mScaleSize; ++i)
	{
		addNoteToKeyMap(mKeyMap, i);
	}
}

void MLScale::clear()
{
	mScaleSize = 0;
	mScaleRatios = {0.};
	
	// index 0 of a scale is always 1/1
	addRatioAsFraction(1, 1);
}

void MLScale::addRatioAsFraction(int n, int d)
{
	addRatio((double)n / (double)d);
}

void MLScale::addRatioAsCents(double cents)
{
	addRatio(pow(2., cents / 1200.));
}

// get the given note frequency as a fraction of the middle note 1/1.
//
double MLScale::middleNoteRatio(int n)
{
	int notesInOctave = static_cast<int>(mKeyMap.mSize - 1);
	int octaveDegree = ml::clamp(mKeyMap.mOctaveScaleDegree, 0, mScaleSize);
	double octaveRatio = mScaleRatios[octaveDegree];

	// get note map index and octave from map.
	int octave, mapIndex;
	int middleRelativeRefNote = n - mKeyMap.mMiddleNote;
	if (middleRelativeRefNote >= 0)
	{
		octave = middleRelativeRefNote / notesInOctave;
		mapIndex = middleRelativeRefNote % notesInOctave;
	}
	else
	{
		octave = ((middleRelativeRefNote + 1) / notesInOctave) - 1;
		mapIndex = notesInOctave - 1 + ((middleRelativeRefNote + 1) % notesInOctave);
	}
	
	// get middle-note relative ratio
	int noteDegree = ml::clamp(mKeyMap.mNoteDegrees[mapIndex], 0, mScaleSize);
	double noteScaleRatio = mScaleRatios[noteDegree];
	double noteOctaveRatio = pow(octaveRatio, octave);
	
	return noteScaleRatio*noteOctaveRatio;
}

// calculate a ratio for each note. key map size, start and end are ignored.
void MLScale::recalcRatiosAndPitches()
{
	// get ratio of reference note to middle note
	double refKeyRatio = middleNoteRatio(mKeyMap.mReferenceNote);
	double refFreqRatio = mKeyMap.mReferenceFreq/(refKeyRatio*440.0);

	for (int i = 0; i < kMLNumNotes; ++i)
	{
		double r = middleNoteRatio(i);
		mRatios[i] = (r*refFreqRatio);
		mPitches[i] = log2(mRatios[i]);
	}
}

void MLScale::loadScaleFromString(const std::string& scaleStr, const std::string& mapStr)
{
	int contentLines = 0;

	std::stringstream fileInputStream(scaleStr);
	std::string inputLine;
	while (std::getline(fileInputStream, inputLine))
	{
		std::stringstream lineInputStream(inputLine);
		const char* descStr = inputLine.c_str();
		if (inputLine[0] != '!') // skip comments
		{
			contentLines++;
			switch(contentLines)
			{
				case 1:
				//	setDescription(descStr);
					break;
				case 2:
					// notes line, unused
					clear();
					break;
				default: // after 2nd line, add ratios.
					if (inputLine.find(".") != std::string::npos)
					{
						// input is a decimal ratio
						double ratio;
						lineInputStream >> ratio;
						addRatioAsCents(ratio);
					}
					else if (inputLine.find("/") != std::string::npos)
					{
						// input is a rational ratio
						int num, denom;
						char slash;
						lineInputStream >> num >> slash >> denom;
						if ((num > 0) && (denom > 0))
						{
							addRatioAsFraction(num, denom);
						}
					}
					else
					{
						// input is an integer, we hope
						int num;
						lineInputStream >> num;
						if (num > 0)
						{
							addRatioAsFraction(num, 1);
						}
					}
					break;
			}
		}
	}
	
	if (mScaleSize > 1)
	{
		int notes = 0;
		if(!mapStr.empty())
		{
			notes = loadMappingFromString(mapStr);
		}
		if(!ml::within(notes, 1, 127))
		{
			setDefaultMapping();
		}
		recalcRatiosAndPitches();
	}
	else
	{
		setDefaultScale();
	}
}

#include <algorithm> 
#include <cctype>
#include <locale>

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
	}));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
	}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

std::string toLower(std::string s) {
	std::transform(s.begin(), s.end(), s.begin(), 
				   [](unsigned char c){ return std::tolower(c); } 
				   );
	return s;
}

// loads .kbm note mapping, as specified at http://www.huygens-fokker.org/scala/help.htm#mappings
// returns the number of notes in the resulting key map.
//
int MLScale::loadMappingFromString(const std::string& mapStr)
{
	int contentLines = 0;
	int notes = 0;
	
	clearKeyMap(mKeyMap);
	std::stringstream fileInputStream(mapStr);
	std::string inputLine;
	std::string trimmedLine;
	std::string whitespace(" \t");

	while (std::getline(fileInputStream, inputLine))
	{
		trimmedLine = (inputLine);
		trim(trimmedLine);
		
		if (trimmedLine[0] != '!') // skip comments
		{
			contentLines++;
			std::stringstream lineInputStream(trimmedLine);

			int unused;
			switch(contentLines)
			{
				case 1:
					lineInputStream >> unused; // size of map
					break;
				case 2:
					lineInputStream >> unused; // start note
					break;
				case 3:
					lineInputStream >> unused; // end note
					break;
				case 4:
					lineInputStream >> mKeyMap.mMiddleNote;
					break;
				case 5:
					lineInputStream >> mKeyMap.mReferenceNote;
					break;
				case 6:
					lineInputStream >> mKeyMap.mReferenceFreq;
					break;
				case 7:
					lineInputStream >> mKeyMap.mOctaveScaleDegree;
					break;
				default: // after 7th content line, add ratios.
				{
					int note = 0;
					if(toLower(trimmedLine) == "x")
					{
						note = kMLUnmappedNote;
					}
					else
					{
						lineInputStream >> note;
					}
										
					addNoteToKeyMap(mKeyMap, note);
					notes++;
				}
				break;
			}
		}
	}
	
	// add octave degree at end of map
	addNoteToKeyMap(mKeyMap, mKeyMap.mOctaveScaleDegree);
	
	return notes;
}


// TODO: look at moving this code and similar kinds to a loader utilities / file helpers object or something.
// the impetus is that an object like MLScale really shouldn't know about Files.
// scales are used by procs, which also shouldn't really have to know about Files. so maybe the MLProc
// gets the i/o utility through the help of its Context.
void MLScale::loadFromRelativePath(ml::Text newPath)
{
	if(!newPath) return;

	// TODO migrate to ml::File
	File scaleRoot(mRootPath);// = getDefaultFileLocation(kScaleFiles);
	
	if (scaleRoot.exists() && scaleRoot.isDirectory())
	{
		// TODO add MLFile methods so this can all be done with MLFiles and ml::Text
		File newFile = scaleRoot.getChildFile(String(newPath.getText()));
		String newName = newFile.getFileNameWithoutExtension();
		File scaleFile = newFile.withFileExtension(".scl");
		if(scaleFile.exists())
		{
			mName = newName.toUTF8();
			String scaleStr = scaleFile.loadFileAsString();
			
			// look for .kbm mapping file
			String mapStr;
			File mappingFile = scaleFile.withFileExtension(".kbm");
			if(mappingFile.exists())
			{
				mapStr = mappingFile.loadFileAsString();
			}
			
			loadScaleFromString(std::string(scaleStr.toUTF8()), std::string(mapStr.toUTF8()));
		}
		else
		{
			// default is 12-equal
			setDefaults();
		}
	}
}


// return the pitch of the given fractional note as log2(p/k), where k = 440Hz.
//
float MLScale::noteToLogPitch(float note) const
{
	if(ml::isNaN(note)) return 0.f;
	
	float fn = ml::clamp(note, 0.f, (float)(kMLNumNotes - 1));
	int i = fn;
	float intPart = i;
	float fracPart = fn - intPart;
	
	float m = 1.f;
	float r0 = mRatios[i];
	float r1 = mRatios[i + 1];
	
	if((r0 > 0.) && (r1 > 0.))
	{
		m = ml::lerp(r0, r1, fracPart);
	}
	else if(r0 > 0.)
	{
		m = r0;
	}
	return log2f(m);
}

float MLScale::quantizePitch(float a) const
{
	float r = 0.f;
	for (int i = kMLNumNotes - 1; i > 0; i--)
	{
		float p = mPitches[i];
		if(p <= a)
		{
			r = p;
			break;
		}
	}
	return r;
}

float MLScale::quantizePitchNearest(float a) const
{
	float r = 1.0f;
	
	// TEMP TODO something faster
	float minD = MAXFLOAT;
	for (int i = 0; i < kMLNumNotes; ++i)
	{
		float p = mPitches[i];
		float d = fabs(p - a);
		if(d <= minD)
		{
			minD = d;
			r = p;
		}

	}
	return r;
}


void MLScale::setName(const std::string& nameStr)
{
	mName = nameStr;
}

void MLScale::setDescription(const std::string& descStr)
{
	mDescription = descStr;
}


// TODO flatten instead

void MLScale::dump()
{
	debug() << "scale " << mName << ":\n";
	for(int i = 0; i<mScaleSize; ++i)
	{
		float r = mScaleRatios[i];
		debug() << "    " << i << " : " << r << "\n";
	}
	debug() << "key map :\n";
	for(int i = 0; i<mKeyMap.mSize; ++i)
	{
		float r = mKeyMap.mNoteDegrees[i];
		debug() << "    " << i << " : " << r << "\n";
	}
	debug() << "ratios:\n";
	for(int i = 0; i<kMLNumNotes; ++i)
	{
		debug() << "    " << i << " : " << mRatios[i] << " / " << mPitches[i] << " (" << mRatios[i]*440. << ") \n";
	}
}



