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

// MLControlEvents are instructions that tell the DSP Engine what to do.
// They can come from different sources such as a score, live code or a live performance.

// a work in progress.

class MLControlEvent
{
public:
    enum ControlEventType
    {
        eNoteOff = 0,
        eNoteOn,
        eController,
        ePitchWheel,
        ePressure,
        ePedal
    };

	MLControlEvent();
	~MLControlEvent() {};
	
	int mChannel;
	float mValue;
	float mVel;
// TODO make MLTime class 	MLTime mTime;
    
};

#endif /* defined(__Aalto__MLControlEvent__) */
