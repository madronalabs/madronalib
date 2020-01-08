/*
 *  mdafactory.cpp
 *  mda-vst3
 *
 *  Created by Arne Scheffler on 6/13/08.
 *
 *  mda VST Plug-ins
 *
 *  Copyright (c) 2008 Paul Kellett
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "public.sdk/source/main/pluginfactory.h"

#include "mdaTrackerController.h"

#include "helpers.h"
#include "version.h"

//-----------------------------------------------------------------------------
bool InitModule () { return true; }
bool DeinitModule () { return true; }

//-----------------------------------------------------------------------------
#define kVersionString	FULL_VERSION_STR

using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF (stringCompanyName, 
				   "http://www.steinberg.net", 
				   "mailto:info@steinberg.de")

//-----------------------------------------------------------------------------
// -- Again
DEF_CLASS2 (INLINE_UID_FROM_FUID (mda::TrackerProcessor::uid),
			PClassInfo::kManyInstances,
			kVstAudioEffectClass,
			"Again",
			Vst::kDistributable,
			Vst::PlugType::kFx,
			kVersionString,
			kVstVersionString,
			mda::TrackerProcessor::createInstance)

DEF_CLASS2 (INLINE_UID_FROM_FUID (mda::TrackerController::uid),
			PClassInfo::kManyInstances,
			kVstComponentControllerClass,
			"Again",
			Vst::kDistributable,
			"",
			kVersionString,
			kVstVersionString,
			mda::TrackerController::createInstance)
//-----------------------------------------------------------------------------
 
END_FACTORY

