
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLScale.h"

MLScale::MLScale()
{
	setDefaultScale();
	setDefaultMapping();
	recalcRatios();
}

MLScale::~MLScale()
{

}

void MLScale::operator= (const MLScale& b)
{
	mName = b.mName;
	mDescription = b.mDescription;
	mRatioList = b.mRatioList;
	for(int p=0; p<kMLNumRatios; ++p)
	{
		mRatios[p] = b.mRatios[p];
		mPitches[p] = b.mPitches[p];
	}
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
		addRatio(100.0 * i);
	}
}

void MLScale::setDefaultMapping()
{
	int scaleSize = mRatioList.size() - 1;
	mKeyMap.mMapSize = scaleSize;
	mKeyMap.mNoteStart = 0;
	mKeyMap.mNoteEnd = 127;
	mKeyMap.mFirstMapNote = 69;
	mKeyMap.mTonicNote = 69;
	mKeyMap.mTonicFreq = 440.0f;
	mKeyMap.mOctaveScaleDegree = scaleSize + 1;
	mKeyMap.mNotes.clear();
	for(int i = 0; i < scaleSize; ++i)
	{
		mKeyMap.mNotes.push_back(i);
	}
}

void MLScale::clear()
{
	mRatioList.clear();
	addRatio(0.0);
}

void MLScale::addRatio(int n, int d)
{
	double ratio = ((double)n / (double)d);
	mRatioList.push_back(ratio);
}

void MLScale::addRatio(double cents)
{
	double ratio = pow(2., cents / 1200.);
	mRatioList.push_back(ratio);
}

// calculate a ratio for each note. key map size, start and end are ignored.
void MLScale::recalcRatios()
{
	int notesInOctave = mKeyMap.mNotes.size();
	if(!notesInOctave) return;

	int ratios = mRatioList.size();
	int octaveIndex = clamp(mKeyMap.mOctaveScaleDegree, 1, ratios - 1);
	double octaveRatio = mRatioList[octaveIndex];
	
	int octave, degree;
	for (int i=0; i < kMLNumRatios; ++i)
	{
		int middleRelativeNote = i - mKeyMap.mTonicNote;
		if (middleRelativeNote >= 0)
		{
			octave = middleRelativeNote / notesInOctave;
			degree = middleRelativeNote % notesInOctave;
		}
		else
		{
			octave = ((middleRelativeNote + 1) / notesInOctave) - 1;
			degree = notesInOctave - 1 + ((middleRelativeNote + 1) % notesInOctave);
		}
		double octaveStartRatio = pow(octaveRatio, (double)octave);
		int ratioIdx = mKeyMap.mNotes[degree];
		ratioIdx = clamp(ratioIdx, 0, ratios - 1);
		mRatios[i] = (float)octaveStartRatio*mRatioList[mKeyMap.mNotes[degree]]*mKeyMap.mTonicFreq/440.0f;
		mPitches[i] = log2f(mRatios[i]);
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
						addRatio(ratio);
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
							addRatio(num, denom);
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
							addRatio(num, 1);
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
		if(!notes)
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

// loads .kbm note mapping, as specified at http://www.huygens-fokker.org/scala/help.htm#mappings
int MLScale::loadMappingFromString(const std::string& mapStr)
{
	int contentLines = 0;
	mKeyMap.mNotes.clear();
	std::stringstream fileInputStream(mapStr);
	std::string inputLine;
	std::string trimmedLine;
	std::string whitespace(" \t");

	while (std::getline(fileInputStream, inputLine))
	{
		trimmedLine = inputLine.substr(inputLine.find_first_not_of(whitespace), inputLine.length());
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
					lineInputStream >> mKeyMap.mFirstMapNote;
					break;
				case 5:
					lineInputStream >> mKeyMap.mTonicNote;
					break;
				case 6:
					lineInputStream >> mKeyMap.mTonicFreq;
					break;
				case 7:
					lineInputStream >> mKeyMap.mOctaveScaleDegree;
					break;
				default: // after 7th content line, add ratios.
				{
					int note = 0;
					lineInputStream >> note;
					mKeyMap.mNotes.push_back(note);
				}
				break;
			}
		}
	}
	return mKeyMap.mNotes.size();
}

// TODO: look at moving this code and similar kinds to a loader utilities / file helpers object or something.
// the impetus is that an object like MLScale really shouldn't know about Files.
// scales are used by procs, which also shouldn't really have to know about Files. so maybe the MLProc
// gets the i/o utility through the help of its Context.
void MLScale::loadFromRelativePath(const std::string& scaleName)
{
	if(scaleName != mScalePath)
	{
		File scaleRoot = getDefaultFileLocation(kScaleFiles);
		if (scaleRoot.exists() && scaleRoot.isDirectory())
		{
			// TODO add MLFile methods so this can all be done with MLFiles and std::string
			File scaleFile = scaleRoot.getChildFile(String(scaleName.c_str())).withFileExtension(".scl");
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
		// use of c_str preventing possible move assignment if (= std::string) were used. TODO revisit!
		mScalePath = scaleName.c_str();
	}
}

float MLScale::noteToPitch(float note) const
{
	float fn = clamp(note, 0.f, (float)(kMLNumScaleNotes - 1));
	int i = fn;
	float intPart = i;
	float fracPart = fn - intPart;
	float a = mRatios[i];
	float b = mRatios[i + 1];
	return lerp(a, b, fracPart) * (mKeyMap.mTonicFreq/440.0f);
}

float MLScale::noteToPitch(int note) const
{
	int n = clamp(note, 0, kMLNumScaleNotes - 1);
	return mRatios[n] * (mKeyMap.mTonicFreq/440.0f); 
}

float MLScale::noteToLogPitch(float note) const
{
	return log2f(noteToPitch(clamp(note, 0.f, 127.f)));
}

float MLScale::quantizePitch(float a) const
{
	float r = 1.0f;
	for (int i = kMLNumRatios - 1; i > 0; i--)
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
	int n = mRatioList.size();
	for(int i = 0; i<n; ++i)
	{
		debug() << "    " << i << " : " << mRatioList[i] << "\n";
	}
}


