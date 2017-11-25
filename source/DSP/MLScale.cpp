
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLScale.h"

MLScale::MLScale()
{
	setDefaults();
}

MLScale::~MLScale()
{

}

void MLScale::operator= (const MLScale& b)
{
	mName = b.mName;
	mDescription = b.mDescription;
	mScaleRatios = b.mScaleRatios;
	
	mNoteIsMapped = b.mNoteIsMapped;
	mRatios = b.mRatios;
	mPitches = b.mPitches;
}

void MLScale::setDefaults()
{
	setDefaultScale();
	setDefaultMapping();
	recalcRatios();
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
	int scaleSize = countRatios() - 1;
	mKeyMap.mMapSize = scaleSize;
	mKeyMap.mNoteStart = 0;
	mKeyMap.mNoteEnd = 127;
	mKeyMap.mMiddleNote = 69;	
	mKeyMap.mReferenceNote = 69; // A3
	mKeyMap.mReferenceFreq = 440.0f;
	mKeyMap.mOctaveScaleDegree = scaleSize; // 1-indexed, as in Scala format

	clearKeyMap(mKeyMap);
	for(int i = 0; i < scaleSize; ++i)
	{
		mKeyMap.mNoteIndexes[i] = i;
	}
}

void MLScale::clear()
{
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
	double r;
	
	int notesInOctave = static_cast<int>(mKeyMap.mMapSize);
	if(!notesInOctave) return 1.f;
	
	int ratios = static_cast<int>(mScaleRatios.size()); // includes beginning 1/1
	int octaveIndex = ml::clamp(mKeyMap.mOctaveScaleDegree, 1, ratios);
	double octaveRatio = mScaleRatios[octaveIndex];

	int octave, degree;
	
	int middleRelativeRefNote = n - mKeyMap.mMiddleNote;
	if (middleRelativeRefNote >= 0)
	{
		octave = middleRelativeRefNote / notesInOctave;
		degree = middleRelativeRefNote % notesInOctave;
	}
	else
	{
		octave = ((middleRelativeRefNote + 1) / notesInOctave) - 1;
		degree = notesInOctave - 1 + ((middleRelativeRefNote + 1) % notesInOctave);
	}
	double octaveStartRefRatio = pow(octaveRatio, (double)octave);
	int ratioIdx = mKeyMap.mNoteIndexes[degree];
	
	if(ml::within(ratioIdx, 0, kMLNumRatios))
	{	
		r = octaveStartRefRatio*mScaleRatios[ratioIdx];
	}
	else
	{
		r = 0.;
	}

	return r;
}

// calculate a ratio for each note. key map size, start and end are ignored.
void MLScale::recalcRatios()
{	
	double refKeyRatio = middleNoteRatio(mKeyMap.mReferenceNote);
	double refFreqRatio = mKeyMap.mReferenceFreq/(refKeyRatio*440.0);
	
	int mapStart = 0;
	int mapEnd = kMLNumRatios - 1;
	for (int i = mapStart; i <= mapEnd; ++i)
	{
		double r = middleNoteRatio(i);
		if(r > 0.)
		{
			mNoteIsMapped[i] = true;
			mRatios[i] = r*refFreqRatio;			
			mPitches[i] = log2(mRatios[i]);
		}
		else
		{
			// unmapped note
			mNoteIsMapped[i] = false;
			mRatios[i] = 0.;
			mPitches[i] = 0.;
		}
	}
}

void MLScale::loadFromString(const std::string& scaleStr, const std::string& mapStr)
{
	int contentLines = 0;
	int ratios = 0;
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
					setDescription(descStr);
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
						ratios++;
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
							ratios++;
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
							ratios++;
							addRatioAsFraction(num, 1);
						}
					}
					break;
			}
		}
	}
	
	if (ratios > 0)
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
		recalcRatios();
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

			switch(contentLines)
			{
				case 1:
					lineInputStream >> mKeyMap.mMapSize;
					break;
				case 2:
					lineInputStream >> mKeyMap.mNoteStart;
					break;
				case 3:
					lineInputStream >> mKeyMap.mNoteEnd;
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
										
					addIndexToKeyMap(mKeyMap, note);
					notes++;
				}
				break;
			}
		}
	}
	
	// pad map if needed
	while(mKeyMap.mNoteIndexes.size() < mKeyMap.mMapSize)
	{
		addIndexToKeyMap(mKeyMap, kMLUnmappedNote);	
	}

	return notes;
}

void MLScale::setRootPath(String root)
{
	mRootPath = root;
}

// TODO: look at moving this code and similar kinds to a loader utilities / file helpers object or something.
// the impetus is that an object like MLScale really shouldn't know about Files.
// scales are used by procs, which also shouldn't really have to know about Files. so maybe the MLProc
// gets the i/o utility through the help of its Context.
void MLScale::loadFromRelativePath(ml::Text newPath)
{
	if(!newPath) return;
	if(newPath != mRelativePath)
	{
		// TODO migrate to ml::File 
		File scaleRoot(mRootPath);// = getDefaultFileLocation(kScaleFiles);
		
		if (scaleRoot.exists() && scaleRoot.isDirectory())
		{
			// TODO add MLFile methods so this can all be done with MLFiles and ml::Text
			File scaleFile = scaleRoot.getChildFile(String(newPath.getText())).withFileExtension(".scl");
			if(scaleFile.exists())
			{
				String scaleStr = scaleFile.loadFileAsString();
				
				// look for .kbm mapping file
				String mapStr;
				File mappingFile = scaleFile.withFileExtension(".kbm");
				if(mappingFile.exists())
				{
					mapStr = mappingFile.loadFileAsString();
				}
				
				loadFromString(std::string(scaleStr.toUTF8()), std::string(mapStr.toUTF8()));
			}
			else
			{
				// default is 12-equal
				setDefaults();
			}
		}
		mRelativePath = newPath;
	}
}

// return the pitch of the given fractional note as a ratio p/k, where k = 440Hz.
//
float MLScale::noteToPitch(float note) const
{
	if(ml::isNaN(note)) return 0.f;
	
	float fn = ml::clamp(note, 0.f, (float)(kMLNumRatios - 1));
	int i = fn;
	float intPart = i;
	float fracPart = fn - intPart;
	
	float m = 1.f;
	if(mNoteIsMapped[i] && mNoteIsMapped[i + 1])
	{
		m = lerp(mRatios[i], mRatios[i + 1], fracPart);
	}
	else if(mNoteIsMapped[i])
	{
		m = mRatios[i];
	}
	return m;
}

// return the pitch of the given note as a ratio p/k, where k = 440Hz.
//
float MLScale::noteToPitch(int note) const
{
	float r = 1.f;
	int n = ml::clamp(note, 0, kMLNumRatios - 1);
	if(mNoteIsMapped[n])
	{
		r = mRatios[n];
	}
	return r;
}

float MLScale::noteToLogPitch(float note) const
{
	return log2f(noteToPitch(note));
}

float MLScale::quantizePitch(float a) const
{
	float r = 0.f;
	for (int i = kMLNumRatios - 1; i > 0; i--)
	{
		if(mNoteIsMapped[i])
		{
			float p = mPitches[i];
			if(p <= a) 
			{
				r = p;
				break;
			}
		}
	}
	return r;
}

float MLScale::quantizePitchNearest(float a) const
{
	float r = 1.0f;
	
	// TEMP TODO something faster
	float minD = MAXFLOAT;
	for (int i = 0; i < kMLNumRatios; ++i)
	{
		if(mNoteIsMapped[i])
		{
			float p = mPitches[i];
			float d = fabs(p - a);
			if(d <= minD) 
			{
				minD = d;
				r = p;
			}
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

void MLScale::dump()
{
	debug() << "scale " << mName << ":\n";
	for(int i = 0; i<kMLNumRatios; ++i)
	{
		if(mNoteIsMapped[i])
		{
			debug() << "    " << i << " : " << mScaleRatios[i] << "\n";
		}
	}
}


