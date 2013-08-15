
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_INPUT_PROTOCOLS_H
#define ML_INPUT_PROTOCOLS_H

enum eInputProtocol
{
	kInputProtocolMIDI = 0,
	kInputProtocolOSC = 1
};

class MLInputProtocolReceiver
{
public:
	MLInputProtocolReceiver() {}
	virtual ~MLInputProtocolReceiver() {}
	virtual void setInputProtocol(int p) = 0;
};

#endif // ML_INPUT_PROTOCOLS_H