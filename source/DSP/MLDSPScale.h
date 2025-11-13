
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#pragma once

#include <vector>
#include <iomanip>
#include <sstream>
#include <array>
#include <cmath>

#include <algorithm>
#include <cctype>
#include <locale>

#include "MLDSPScalarMath.h"

namespace ml
{

// number of notes to calculate. While only 1-127 are needed for MIDI notes,
// the higher ones are used in Virta to quantize partials.
const int kMLNumNotes = 256;

const int kMLUnmappedNote = kMLNumNotes + 1;

class Scale
{
 public:
  Scale()
  {
    setDefaultScale();
    setDefaultMapping();
    recalcRatiosAndPitches();
  }

  ~Scale() = default;

  void operator=(const Scale& b)
  {
    scaleRatios_ = b.scaleRatios_;
    pitches_ = b.pitches_;
  }

  // load a scale from an input string along with an optional mapping.
  void loadScaleFromString(const std::string& scaleStr, const std::string& mapStr = "")
  {
    int contentLines = 0;

    std::stringstream fileInputStream(scaleStr);
    std::string inputLine;
    while (std::getline(fileInputStream, inputLine))
    {
      std::stringstream lineInputStream(inputLine);
      const char* descStr = inputLine.c_str();
      if (inputLine[0] != '!')  // skip comments
      {
        contentLines++;
        switch (contentLines)
        {
          case 1:
            //  setDescription(descStr);
            break;
          case 2:
            // notes line, unused
            clear();
            break;
          default:  // after 2nd line, add ratios.
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

    if (scaleSize_ > 1)
    {
      int notes = 0;
      if (!mapStr.empty())
      {
        notes = loadMappingFromString(mapStr);
      }
      if (!ml::within(notes, 1, 127))
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

  // return pitch of the given note in log pitch (1.0 per octave) space with 440.0Hz = 0.
  // return the pitch of the given fractional note as log2(p/k), where k = 440Hz.
  //
  float noteToLogPitch(float note) const
  {
    if (ml::isNaN(note)) return 0.f;

    float fn = ml::clamp(note, 0.f, (float)(kMLNumNotes - 1));
    int i = (int)fn;
    double intPart = (double)i;
    double fracPart = fn - intPart;

    double m = 1.f;
    double r0 = ratios_[i];
    double r1 = ratios_[i + 1];

    if ((r0 > 0.) && (r1 > 0.))
    {
      m = ml::lerp(r0, r1, fracPart);
    }
    else if (r0 > 0.)
    {
      m = r0;
    }
    return log2f((float)m);
  }

  // return log pitch of the note of the current scale just below the input.
  float quantizePitch(float a) const
  {
    float r = 0.f;
    for (int i = kMLNumNotes - 1; i > 0; i--)
    {
      float p = (float)pitches_[i];
      if (p <= a)
      {
        r = p;
        break;
      }
    }
    return r;
  }

  // return log pitch of the note of the current scale closest to the input.
  float quantizePitchNearest(float a) const
  {
    float fLower{0};
    float fHigher{0};
    int lowerIdx{0};
    for (int i = kMLNumNotes - 1; i > 0; i--)
    {
      float p = (float)pitches_[i];
      if (p <= a)
      {
        fLower = p;
        if (i < kMLNumNotes - 1)
        {
          fHigher = (float)pitches_[i + 1];
        }
        lowerIdx = i;
        break;
      }
    }

    if (lowerIdx == kMLNumNotes - 1)
    {
      return fLower;
    }
    else if (lowerIdx <= 0)
    {
      return (float)pitches_[0];
    }

    float d1 = (a - fLower);
    float d2 = (fHigher - a);
    if (d1 < d2)
    {
      return fLower;
    }
    else
    {
      return fHigher;
    }
  }

  void setName(const std::string& nameStr) { name_ = nameStr; }

  void setDescription(const std::string& descStr) { description_ = descStr; }

  /*
   #include <iostream>
  void dump()
  {
    debug() << "scale " << mName << ":\n";
    for(int i = 0; i<scaleSize_; ++i)
    {
      float r = scaleRatios_[i];
      debug() << "    " << i << " : " << r << "\n";
    }
    debug() << "key map :\n";
    for(int i = 0; i<keyMap_.mSize; ++i)
    {
      float r = keyMap_.mNoteDegrees[i];
      debug() << "    " << i << " : " << r << "\n";
    }
    debug() << "ratios:\n";
    for(int i = 0; i<kMLNumNotes; ++i)
    {
      debug() << "    " << i << " : " << mRatios[i] << " / " << mPitches[i] << " (" <<
  mRatios[i]*440. << ") \n";
    }
  }
   */

 private:
  float noteToPitch(float note) const;

  void addRatioAsFraction(int n, int d) { addRatio((double)n / (double)d); }

  void addRatioAsCents(double cents) { addRatio(std::pow(2., cents / 1200.)); }

  // get the given note frequency as a fraction of the middle note 1/1.
  //
  double middleNoteRatio(int n)
  {
    int notesInOctave = static_cast<int>(keyMap_.size_ - 1);
    int octaveDegree = ml::clamp(keyMap_.octaveScaleDegree_, 0, scaleSize_);
    double octaveRatio = scaleRatios_[octaveDegree];

    // get note map index and octave from map.
    int octave, mapIndex;
    int middleRelativeRefNote = n - keyMap_.middleNote_;
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
    int noteDegree = ml::clamp(keyMap_.noteDegrees_[mapIndex], 0, scaleSize_);
    double noteScaleRatio = scaleRatios_[noteDegree];
    double noteOctaveRatio = std::pow(octaveRatio, octave);

    return noteScaleRatio * noteOctaveRatio;
  }

  // calculate a ratio for each note. key map size, start and end are ignored.
  void recalcRatiosAndPitches()
  {
    // get ratio of reference note to middle note
    double refKeyRatio = middleNoteRatio(keyMap_.referenceNote_);
    double refFreqRatio = keyMap_.referenceFreq_ / (refKeyRatio * 440.0);

    for (int i = 0; i < kMLNumNotes; ++i)
    {
      double r = middleNoteRatio(i);
      ratios_[i] = (r * refFreqRatio);
      pitches_[i] = std::log2(ratios_[i]);
    }
  }

  // trim from start (in place)
  static inline void ltrim(std::string& s)
  {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
  }

  // trim from end (in place)
  static inline void rtrim(std::string& s)
  {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
            s.end());
  }

  // trim from both ends (in place)
  static inline void trim(std::string& s)
  {
    ltrim(s);
    rtrim(s);
  }

  std::string toLower(std::string s)
  {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
  }

  // loads .kbm note mapping, as specified at http://www.huygens-fokker.org/scala/help.htm#mappings
  // returns the number of notes in the resulting key map.
  //
  int loadMappingFromString(const std::string& mapStr)
  {
    int contentLines = 0;
    int notes = 0;

    clearKeyMap(keyMap_);
    std::stringstream fileInputStream(mapStr);
    std::string inputLine;
    std::string trimmedLine;
    std::string whitespace(" \t");

    while (std::getline(fileInputStream, inputLine))
    {
      trimmedLine = (inputLine);
      trim(trimmedLine);

      if (trimmedLine[0] != '!')  // skip comments
      {
        contentLines++;
        std::stringstream lineInputStream(trimmedLine);

        int unused;
        switch (contentLines)
        {
          case 1:
            lineInputStream >> unused;  // size of map
            break;
          case 2:
            lineInputStream >> unused;  // start note
            break;
          case 3:
            lineInputStream >> unused;  // end note
            break;
          case 4:
            lineInputStream >> keyMap_.middleNote_;
            break;
          case 5:
            lineInputStream >> keyMap_.referenceNote_;
            break;
          case 6:
            lineInputStream >> keyMap_.referenceFreq_;
            break;
          case 7:
            lineInputStream >> keyMap_.octaveScaleDegree_;
            break;
          default:  // after 7th content line, add ratios.
          {
            int note = 0;
            if (toLower(trimmedLine) == "x")
            {
              note = kMLUnmappedNote;
            }
            else
            {
              lineInputStream >> note;
            }

            addNoteToKeyMap(keyMap_, note);
            notes++;
          }
          break;
        }
      }
    }

    // add octave degree at end of map
    addNoteToKeyMap(keyMap_, keyMap_.octaveScaleDegree_);

    return notes;
  }

  void clear()
  {
    scaleSize_ = 0;
    scaleRatios_ = {0.};

    // index 0 of a scale is always 1/1
    addRatioAsFraction(1, 1);
  }

  void setDefaultScale()
  {
    clear();
    setName("12-equal");
    setDescription("The chromatic equal-tempered scale.");
    // make 12-ET scale
    for (int i = 1; i <= 12; ++i)
    {
      addRatioAsCents(100.0 * i);
    }
  }

  void setDefaultMapping()
  {
    clearKeyMap(keyMap_);

    keyMap_.middleNote_ = 69;     // arbitrary in equal-tempered scale
    keyMap_.referenceNote_ = 69;  // A3
    keyMap_.referenceFreq_ = 440.0f;
    keyMap_.octaveScaleDegree_ = scaleSize_ - 1;

    for (int i = 0; i < scaleSize_; ++i)
    {
      addNoteToKeyMap(keyMap_, i);
    }
  }

  // key map structure and utility functions

  struct keyMap
  {
    int size_;

    // Middle note where the first entry of the mapping is placed
    int middleNote_;

    // note that is defined to be the reference frequency
    int referenceNote_;

    // reference frequency
    float referenceFreq_;

    // Scale degree to consider as formal octave
    int octaveScaleDegree_;

    // scale degree for each note
    std::array<int, kMLNumNotes> noteDegrees_;
  };

  inline void clearKeyMap(keyMap& map)
  {
    map.noteDegrees_.fill(-1);
    map.size_ = 0;
  }

  inline void addNoteToKeyMap(keyMap& map, int newIdx)
  {
    if (map.size_ < kMLNumNotes)
    {
      map.noteDegrees_[map.size_++] = newIdx;
    }
  }

  keyMap keyMap_;

  std::string name_;
  std::string description_;

  // list of ratios forming a scale.  The first entry is always 1.0, or 0 cents.
  // The last entry is the ratio of an octave, typically but not always 2.

  // TODO use Ratios
  std::array<double, kMLNumNotes> scaleRatios_;

  int scaleSize_;
  inline void addRatio(double newRatio)
  {
    if (scaleSize_ < kMLNumNotes)
    {
      scaleRatios_[scaleSize_++] = newRatio;
    }
  }

  // pitch for each integer note number stored in as a ratio p/k where k = 440.0 Hz
  std::array<double, kMLNumNotes> ratios_;

  // pitch for each integer note number stored in linear octave space. pitch = log2(ratio).
  std::array<double, kMLNumNotes> pitches_;
};

}  // namespace ml
