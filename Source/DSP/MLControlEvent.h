//
//  MLControlEvent.h
//  Aalto
//
//  Created by Randy Jones on 7/9/14.
//
//

#ifndef __Aalto__MLControlEvent__
#define __Aalto__MLControlEvent__

#include <iostream>
#include <vector>

// MLControlEvents are instructions that tell the DSP Engine what to do.
// They can come from different sources such as a score, live code or a live performance.

class MLControlEvent
{
public:
    enum EventType
    {
        eNull = 0,
        eNoteOff,
        eNoteOn,
        eController,
        ePitchWheel,
        eNotePressure,
        eChannelPressure,
        eProgramChange,
        eSustainPedal
    };

	MLControlEvent();
	MLControlEvent(EventType type, int channel, int id, int time, float value, float value2);
	~MLControlEvent();
    void clear();
    bool isFree() const {return mType == eNull;}
	
    EventType mType;
	int mChannel;
    int mID; // the MIDI key or touch number that created the event. a note-off can match a note-on by ID.
	float mValue1;
	float mValue2;
    // TODO make MLTime class and use global timestamp in events. Currently used as sample offset from block start.
    int mTime;
};

const MLControlEvent kMLNullControlEvent;

class MLControlEventVector :
    public std::vector<MLControlEvent>
{
public:
    int findFreeEvent() const;
};



#endif /* defined(__Aalto__MLControlEvent__) */
