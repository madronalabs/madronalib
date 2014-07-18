//
//  MLControlEvent.cpp
//  Aalto
//
//  Created by Randy Jones on 7/9/14.
//:
//

#include "MLControlEvent.h"

MLControlEvent::MLControlEvent() :
    mType(eNull),
    mChannel(0),
    mID(0),
    mValue1(0.),
    mValue2(0.),
    mTime(0)
{
}

MLControlEvent::MLControlEvent(MLControlEvent::EventType type, int channel, int id, int time, float value, float value2) :
    mType(type),
    mChannel(channel),
    mID(id),
    mTime(time),
    mValue1(value),
    mValue2(value2)
{
}

MLControlEvent::~MLControlEvent()
{
}

void MLControlEvent::clear()
{
    *this = kMLNullControlEvent;
}

int MLControlEventVector::findFreeEvent() const
{
    int r = -1;
    for(int i=0; i<size(); ++i)
    {
        if (at(i).isFree())
        {
            r = i;
            break;
        }
    }
    return r;
}
