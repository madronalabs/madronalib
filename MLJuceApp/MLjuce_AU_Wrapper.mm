/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#ifdef ML_BUILD_AU

#include <Cocoa/Cocoa.h>

#include "JuceHeader.h"
#include "MLjuce_IncludeCharacteristics.h"
#include <AudioUnit/AUCocoaUIView.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include "MusicDeviceBase.h"

// ML
#define Button juce::Button
#define Point juce::Point
#define Component juce::Component
#include "MLPluginProcessor.h"
#include "MLPluginEditor.h"
// ML

//==============================================================================
#define juceFilterObjectPropertyID 0x1a45ffe9
static Array<void*> activePlugins, activeUIs;

static const short channelConfigs[][2] = { JucePlugin_PreferredChannelConfigurations };
static const int numChannelConfigs = sizeof (channelConfigs) / sizeof (*channelConfigs);

#if JucePlugin_IsSynth
 #define JuceAUBaseClass MusicDeviceBase
#else
 #define JuceAUBaseClass AUMIDIEffectBase
#endif

//==============================================================================
/** Somewhere in the codebase of your plugin, you need to implement this function
    and make it create an instance of the filter subclass that you're building.
*/
// ML
extern MLPluginProcessor* JUCE_CALLTYPE createPluginFilter();
// ML

//==============================================================================
#define appendMacro1(a, b, c, d) a ## _ ## b ## _ ## c ## _ ## d
#define appendMacro2(a, b, c, d) appendMacro1(a, b, c, d)
#define MakeObjCClassName(rootName)  appendMacro2 (rootName, JUCE_MAJOR_VERSION, JUCE_MINOR_VERSION, JucePlugin_AUExportPrefix)

#define JuceUICreationClass         JucePlugin_AUCocoaViewClassName
#define JuceUIViewClass             MakeObjCClassName(JuceUIViewClass)

class JuceAU;
class EditorCompHolder;

//==============================================================================
@interface JuceUICreationClass   : NSObject <AUCocoaUIBase>
{
}

- (JuceUICreationClass*) init;
- (void) dealloc;
- (unsigned) interfaceVersion;
- (NSString *) description;
- (NSView*) uiViewForAudioUnit: (AudioUnit) inAudioUnit
                      withSize: (NSSize) inPreferredSize;
@end

/*
//==============================================================================
@interface JuceUIViewClass : NSView
{
    AudioProcessor* filter;
    JuceAU* au;
    EditorCompHolder* editorComp;
}

- (JuceUIViewClass*) initWithFilter: (AudioProcessor*) filter
                             withAU: (JuceAU*) au
                      withComponent: (AudioProcessorEditor*) editorComp;
- (void) dealloc;
- (void) shutdown;
- (void) applicationWillTerminate: (NSNotification*) aNotification;
- (void) viewDidMoveToWindow;
- (BOOL) mouseDownCanMoveWindow;
- (void) filterBeingDeleted: (JuceAU*) au_;
- (void) deleteEditor;

@end
*/

//==============================================================================
@interface JuceUIViewClass : NSView
{
    AudioProcessor* filter;
    JuceAU* au;
    EditorCompHolder* editorComp;
}

- (JuceUIViewClass*) initWithFilter: (AudioProcessor*) filter
                             withAU: (JuceAU*) au
                      withComponent: (AudioProcessorEditor*) editorComp;
- (void) dealloc;
- (void) shutdown;
- (void) applicationWillTerminate: (NSNotification*) aNotification;
- (void) viewDidMoveToWindow;
- (BOOL) mouseDownCanMoveWindow;
- (void) filterBeingDeleted: (JuceAU*) au_;
- (void) deleteEditor;

- (BOOL) acceptsFirstResponder;
- (BOOL) becomeFirstResponder;
- (BOOL) resignFirstResponder;
- (BOOL) canBecomeKeyWindow;
- (BOOL) acceptsFirstMouse: (NSEvent*) ev;
- (void) keyDown: (NSEvent*) ev;
- (void) keyUp: (NSEvent*) ev;

@end



//==============================================================================
class JuceAU   : public JuceAUBaseClass,
				public MLAudioProcessorListener, // ML
				public AudioProcessorListener,
				public AudioPlayHead,
				public ComponentListener
{
public:
    //==============================================================================
    JuceAU (AudioUnit component)
      #if JucePlugin_IsSynth
        : MusicDeviceBase (component, 0, 1),
      #else
        : AUMIDIEffectBase (component),
      #endif
          bufferSpace (2, 16),
          prepared (false)
    {
        if (activePlugins.size() + activeUIs.size() == 0)
        {
          #if BUILD_AU_CARBON_UI
            NSApplicationLoad();
          #endif

            initialiseJuce_GUI();
        }

        juceFilter = createPluginFilter();
        jassert (juceFilter != nullptr);

		juceFilter->setWrapperFormat(MLPluginFormats::eAUPlugin);
        juceFilter->setPlayHead (this);
        juceFilter->addListener (this);
 		juceFilter->setMLListener (this); 

        Globals()->UseIndexedParameters (juceFilter->getNumParameters());

        activePlugins.add (this);

        zerostruct (auEvent);
        auEvent.mArgument.mParameter.mAudioUnit = GetComponentInstance();
        auEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
        auEvent.mArgument.mParameter.mElement = 0;
    }

    ~JuceAU()
    {
        for (int i = activeUIs.size(); --i >= 0;)
            [((JuceUIViewClass*) activeUIs.getUnchecked(i)) filterBeingDeleted: this];

        juceFilter = nullptr;

        jassert (activePlugins.contains (this));
        activePlugins.removeFirstMatchingValue (this);

        if (activePlugins.size() + activeUIs.size() == 0)
            shutdownJuce_GUI();
    }

    //==============================================================================
    ComponentResult GetPropertyInfo (AudioUnitPropertyID inID,
                                     AudioUnitScope inScope,
                                     AudioUnitElement inElement,
                                     UInt32& outDataSize,
                                     Boolean& outWritable)
    {
        if (inScope == kAudioUnitScope_Global)
        {
            if (inID == juceFilterObjectPropertyID)
            {
                outWritable = false;
                outDataSize = sizeof (void*) * 2;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_OfflineRender)
            {
                outWritable = true;
                outDataSize = sizeof (UInt32);
                return noErr;
            }
            else if (inID == kMusicDeviceProperty_InstrumentCount)
            {
                outDataSize = sizeof (UInt32);
                outWritable = false;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_CocoaUI)
            {
              #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
                // (On 10.4, there's a random obj-c dispatching crash when trying to load a cocoa UI)
                if (SystemStats::getOSXMinorVersionNumber() > 4)
              #endif
                {
                    outDataSize = sizeof (AudioUnitCocoaViewInfo);
                    outWritable = true;
                    return noErr;
                }
            }
		
 			// ----------------------------------------------------------------
			// ML
			else if(inID == kAudioUnitProperty_ParameterClumpName) 
			{
				outDataSize = sizeof(AudioUnitParameterNameInfo);
				outWritable = false;
				return noErr;
			}
			
			else if(inID == kAudioUnitProperty_SampleRate) 
			{
				outDataSize = sizeof(Float64);
				outWritable = true;
				return noErr;
			}
			// ML
			// ----------------------------------------------------------------
       }

        return JuceAUBaseClass::GetPropertyInfo (inID, inScope, inElement, outDataSize, outWritable);
    }

    ComponentResult GetProperty (AudioUnitPropertyID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 void* outData)
    {
        if (inScope == kAudioUnitScope_Global)
        {
            if (inID == juceFilterObjectPropertyID)
            {
                ((void**) outData)[0] = (void*) static_cast <AudioProcessor*> (juceFilter);
                ((void**) outData)[1] = (void*) this;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_OfflineRender)
            {
                *(UInt32*) outData = (juceFilter != nullptr && juceFilter->isNonRealtime()) ? 1 : 0;
                return noErr;
            }
            else if (inID == kMusicDeviceProperty_InstrumentCount)
            {
                *(UInt32*) outData = 1;
                return noErr;
            }
            else if (inID == kAudioUnitProperty_CocoaUI)
            {
              #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
                // (On 10.4, there's a random obj-c dispatching crash when trying to load a cocoa UI)
                if (SystemStats::getOSXMinorVersionNumber() > 4)
              #endif
                {
                    JUCE_AUTORELEASEPOOL

                    AudioUnitCocoaViewInfo* info = (AudioUnitCocoaViewInfo*) outData;

                    const File bundleFile (File::getSpecialLocation (File::currentApplicationFile));
                    NSString* bundlePath = [NSString stringWithUTF8String: (const char*) bundleFile.getFullPathName().toUTF8()];
                    NSBundle* b = [NSBundle bundleWithPath: bundlePath];

                    info->mCocoaAUViewClass[0] = (CFStringRef) [[[JuceUICreationClass class] className] retain];
                    info->mCocoaAUViewBundleLocation = (CFURLRef) [[NSURL fileURLWithPath: [b bundlePath]] retain];

                    return noErr;
                }
            }
		
			
			// ----------------------------------------------------------------
			// ML
			else if (inID == kAudioUnitProperty_ParameterClumpName)
			{
				OSStatus result = noErr;
				AudioUnitParameterNameInfo* ioClumpInfo = (AudioUnitParameterNameInfo*) outData;
				unsigned index = ioClumpInfo->inID;
				
				if (index == kAudioUnitClumpID_System)	// this ID value is reserved
				{
					result = kAudioUnitErr_InvalidPropertyValue;
				}
				else
				{
					int maxLen = ioClumpInfo->inDesiredLength;
					std::string groupStr (juceFilter->getParameterGroupName(index));
					if (groupStr.length() > 0)
					{
						if (groupStr.length() > maxLen)
						{
							groupStr.resize(maxLen);
						}
						const char* c = groupStr.c_str();
						if (c)
						{		
							CFStringRef clumpName = CFStringCreateWithCString(NULL, c, kCFStringEncodingUTF8);
							CFRetain(clumpName);
							ioClumpInfo->outName = (clumpName);
						}
					}
				}
				return result;
			}
			
			// sample rate
			else if (inID == kAudioUnitProperty_SampleRate)
			{				
                *(Float64*) outData = juceFilter->getSampleRate();
                return noErr;
			}
			// ML
			// ----------------------------------------------------------------

        }

        return JuceAUBaseClass::GetProperty (inID, inScope, inElement, outData);
    }

    ComponentResult SetProperty (AudioUnitPropertyID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 const void* inData,
                                 UInt32 inDataSize)
    {
        if (inScope == kAudioUnitScope_Global && inID == kAudioUnitProperty_OfflineRender)
        {
            if (juceFilter != nullptr)
                juceFilter->setNonRealtime ((*(UInt32*) inData) != 0);

            return noErr;
        }

        return JuceAUBaseClass::SetProperty (inID, inScope, inElement, inData, inDataSize);
    }

    ComponentResult SaveState (CFPropertyListRef* outData)
    {
        ComponentResult err = JuceAUBaseClass::SaveState (outData);

        if (err != noErr)
            return err;

        jassert (CFGetTypeID (*outData) == CFDictionaryGetTypeID());

        CFMutableDictionaryRef dict = (CFMutableDictionaryRef) *outData;

		// ----------------------------------------------------------------
		// ML
		// get saved preset name
		char nameBuf[33];
		CFStringRef kNameString = CFSTR(kAUPresetNameKey);
		CFStringRef presetName = reinterpret_cast<CFStringRef>(CFDictionaryGetValue (dict, kNameString));		
		const bool nameResult = (CFStringGetCString(presetName, nameBuf, 32, kCFStringEncodingASCII));
		// ML
		// ----------------------------------------------------------------

        if (juceFilter != nullptr)
        {
            juce::MemoryBlock state;
            juceFilter->getCurrentProgramStateInformation (state);

			// ----------------------------------------------------------------
			// ML
			if (nameResult)
			{ 
				// update filter with saved name.
				juceFilter->setCurrentPresetName (nameBuf);
			}
			// ML
			// ----------------------------------------------------------------

            if (state.getSize() > 0)
            {
                CFDataRef ourState = CFDataCreate (kCFAllocatorDefault, (const UInt8*) state.getData(), state.getSize());
                CFDictionarySetValue (dict, CFSTR("jucePluginState"), ourState);
                CFRelease (ourState);
            }
        }

        return noErr;
    }

    ComponentResult RestoreState (CFPropertyListRef inData)
    {
        ComponentResult err = JuceAUBaseClass::RestoreState (inData);

        if (err != noErr)
            return err;
			
		// ----------------------------------------------------------------
		// ML
		// get saved preset name
		char nameBuf[33];
        CFDictionaryRef dict = (CFDictionaryRef) inData;
		CFStringRef kNameString = CFSTR(kAUPresetNameKey);
		CFStringRef presetName = reinterpret_cast<CFStringRef>(CFDictionaryGetValue (dict, kNameString));		
		const bool nameResult = (CFStringGetCString(presetName, nameBuf, 32, kCFStringEncodingASCII));

		// ML
		// ----------------------------------------------------------------

        if (juceFilter != nullptr)
        {
            CFDictionaryRef dict = (CFDictionaryRef) inData;
            CFDataRef data = 0;

            if (CFDictionaryGetValueIfPresent (dict, CFSTR("jucePluginState"), (const void**) &data))
            {
                if (data != 0)
                {
                    const int numBytes = (int) CFDataGetLength (data);
                    const JUCE_NAMESPACE::uint8* const rawBytes = CFDataGetBytePtr (data);

                    if (numBytes > 0)
                        juceFilter->setCurrentProgramStateInformation (rawBytes, numBytes);
					// ----------------------------------------------------------------
					// ML
					if (nameResult)
					{
						juceFilter->setCurrentPresetName (nameBuf);
					}
					// ML
					// ----------------------------------------------------------------
               }
            }
        }

        return noErr;
    }

// --------------------------------------------------------------------------------
#pragma mark -
#pragma mark MLAudioProcessorListener methods
// this code ended up in here because of access to RestoreState().  There may be other issues.
// TODO look at moving this to MLJuceFilesMac.
	
	void loadFile(const File& f) 
	{
		String juceName = f.getFullPathName();
		const char* fileStr = juceName.toUTF8();
		debug() << JucePlugin_Name << " AU: loading state from file: " << fileStr << "\n";

		CFPropertyListRef propertyList;
		CFStringRef       errorString;
		CFDataRef         resourceData;
		Boolean           status;
		SInt32            errorCode;
		ComponentResult err;

		// get URL from Juce File
		CFURLRef fileURL = CFURLCreateWithFileSystemPath(NULL, CFStringCreateWithCString(NULL, fileStr, kCFStringEncodingUTF8), kCFURLPOSIXPathStyle, false);

		// Read the CFData file containing the encoded XML.
		status = CFURLCreateDataAndPropertiesFromResource(
				   kCFAllocatorDefault,
				   fileURL,
				   &resourceData,            // place to put file data
				   NULL,
				   NULL,
				   &errorCode);

		// Reconstitute the dictionary using the XML data.
		propertyList = CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
				   resourceData,
				   kCFPropertyListImmutable,
				   &errorString);
				   
		if (errorString == NULL)
		{
			err = RestoreState (propertyList);
			if (err != noErr)
			{
				MLError() << "error: " << err << "loading preset file.\n";
			}
		}
		else
		{
			char errBuf[256];
			const bool errResult = (CFStringGetCString(errorString, errBuf, 255, kCFStringEncodingASCII));
			if (errResult)
			{
				std::cout << errBuf << "\n";
				CFRelease( errorString );
			}
		}
		
		CFRelease( resourceData );
		CFRelease( propertyList );

	}


	void saveToFile(const File& f) 
	{
		String juceName = f.getFullPathName();
		const char* fileStr = juceName.toUTF8();
		String shortName = f.getFileNameWithoutExtension();
	
		// get URL from Juce File
		CFURLRef fileURL = CFURLCreateWithFileSystemPath(NULL, CFStringCreateWithCString(NULL, fileStr, kCFStringEncodingUTF8), kCFURLPOSIXPathStyle, false);
		
		// get property list. 
		CFPropertyListRef myPropsRef;
		ComponentResult err = JuceAUBaseClass::SaveState (&myPropsRef);
        if (err != noErr) return;
        jassert (CFGetTypeID (myPropsRef) == CFDictionaryGetTypeID());
        CFMutableDictionaryRef dict = (CFMutableDictionaryRef) myPropsRef;
	
		// save state to the dictionary.
		if (juceFilter != 0)
        {
            juce::MemoryBlock state;
            juceFilter->getCurrentProgramStateInformation (state);
		
            if (state.getSize() > 0)
            {
                CFDataRef ourState = CFDataCreate (kCFAllocatorDefault, (const UInt8*) state.getData(), state.getSize());
                CFDictionarySetValue (dict, CFSTR("jucePluginState"), ourState);
                CFRelease (ourState);
            }
        }

		// overwrite preset name in properties
		CFStringRef newName = CFStringCreateWithCString(NULL, shortName.toUTF8(), kCFStringEncodingUTF8);
		CFStringRef kNameString = CFSTR(kAUPresetNameKey);
		CFDictionarySetValue (dict, kNameString, newName);
		
		// Convert the property list into XML data.
		CFDataRef xmlCFDataRef = CFPropertyListCreateXMLData(kCFAllocatorDefault, myPropsRef );
		SInt32 errorCode = coreFoundationUnknownErr;
		
		if (NULL != xmlCFDataRef)
		{
			// Write the XML data to the CFData file.
			(void) CFURLWriteDataAndPropertiesToResource(fileURL, xmlCFDataRef, NULL, &errorCode);
		
			// Release the XML data
			CFRelease(xmlCFDataRef);
		}
		
		// restore state in base class, to update preset name in host.
		JuceAUBaseClass::RestoreState (myPropsRef);
		CFRelease(myPropsRef);
	}

	
#pragma mark -
	// --------------------------------------------------------------------------------


    UInt32 SupportedNumChannels (const AUChannelInfo** outInfo)
    {
        // You need to actually add some configurations to the JucePlugin_PreferredChannelConfigurations
        // value in your JucePluginCharacteristics.h file..
        jassert (numChannelConfigs > 0);

        if (outInfo != nullptr)
        {
            *outInfo = channelInfo;

            for (int i = 0; i < numChannelConfigs; ++i)
            {
              #if JucePlugin_IsSynth
                channelInfo[i].inChannels = 0;
              #else
                channelInfo[i].inChannels = channelConfigs[i][0];
              #endif
                channelInfo[i].outChannels = channelConfigs[i][1];
            }
        }

        return numChannelConfigs;
    }

    //==============================================================================
    ComponentResult GetParameterInfo (AudioUnitScope inScope,
                                      AudioUnitParameterID inParameterID,
                                      AudioUnitParameterInfo& outParameterInfo)
    {
        const int index = (int) inParameterID;
		
		// TEMP lots of testing printouts for debugging
		
		MLPublishedParamPtr paramPtr; // ML
		paramPtr = juceFilter->getParameterPtr(index); // ML
		
		if (!paramPtr)
		{
			debug() << "parameter #" << index << ": null parameter ptr!\n";
			return kAudioUnitErr_InvalidParameter;
		}
		
		if (inScope != kAudioUnitScope_Global)
		{
			debug() << "parameter #" << index << ": invalid scope!\n";
		}
		
		if (juceFilter == nullptr)
		{
			debug() << "parameter #" << index << ": no audio processor!\n";
		}
		
		if (index >= juceFilter->getNumParameters())
		{
			debug() << "parameter #" << index << ": index too big, max " << juceFilter->getNumParameters() << "!\n";
		}
		
		
        if (inScope == kAudioUnitScope_Global
             && juceFilter != nullptr
             && index < juceFilter->getNumParameters())
        {
            outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable
                                      | kAudioUnitParameterFlag_IsReadable
                                      | kAudioUnitParameterFlag_HasCFNameString;

            const String name (juceFilter->getParameterName (index));

            // set whether the param is automatable (unnamed parameters aren't allowed to be automated)
            if (name.isEmpty() || ! juceFilter->isParameterAutomatable (index))
                outParameterInfo.flags |= kAudioUnitParameterFlag_NonRealTime;

            if (juceFilter->isMetaParameter (index))
                outParameterInfo.flags |= kAudioUnitParameterFlag_IsGlobalMeta;

            AUBase::FillInParameterName (outParameterInfo,
                                         name.toCFString(),
                                         false);

			// ----------------------------------------------------------------
			// ML
			outParameterInfo.minValue = paramPtr->getRangeLo();
			outParameterInfo.maxValue = paramPtr->getRangeHi();
            outParameterInfo.defaultValue = paramPtr->getDefault();		
			outParameterInfo.unit = paramPtr->getUnit(); // unimplemented

			if (paramPtr->getWarpMode() == kJucePluginParam_Exp)
			{
				outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayLogarithmic; 
			}	
			if (paramPtr->getGroupIndex() >= 0)
			{
				// sets kAudioUnitParameterFlag_HasClump			
				HasClump (outParameterInfo, paramPtr->getGroupIndex());
			}		

	/*
	debug() << "parameter #" << index << " (" << paramPtr->getAlias() << ") : ";
	debug() << "min " << outParameterInfo.minValue;
	debug() << " max " << outParameterInfo.maxValue;
	debug() << " dflt " << outParameterInfo.defaultValue;
	debug() << " unit " << outParameterInfo.unit;
	debug() << " flgs " << outParameterInfo.flags;
	debug() << "\n";
	*/
	
			// Another important issue here is to make sure that if a parameter sets 
			// kAudioUnitParameterFlag_ValuesHaveStrings, not only should you support
			// kAudioUnitProperty_ParameterStringFromValue (formerly known as kAudioUnitProperty_ParameterValueName) 
			// for that parameter, but also implement the reverse transformation via 
			// kAudioUnitProperty_ParameterValueFromString to allow the host application 
			// to translate user-entered text into a parameter value.
			// ML
			// ----------------------------------------------------------------
	
            return noErr;
        }
        else
        {
            return kAudioUnitErr_InvalidParameter;
        }
    }

    ComponentResult GetParameter (AudioUnitParameterID inID,
                                  AudioUnitScope inScope,
                                  AudioUnitElement inElement,
                                  Float32& outValue)
    {
        if (inScope == kAudioUnitScope_Global && juceFilter != nullptr)
        {
            outValue = juceFilter->getParameter ((int) inID);
            return noErr;
        }

        return AUBase::GetParameter (inID, inScope, inElement, outValue);
    }

    ComponentResult SetParameter (AudioUnitParameterID inID,
                                  AudioUnitScope inScope,
                                  AudioUnitElement inElement,
                                  Float32 inValue,
                                  UInt32 inBufferOffsetInFrames)
    {
        if (inScope == kAudioUnitScope_Global && juceFilter != nullptr)
        {
            juceFilter->setParameter ((int) inID, inValue);
            return noErr;
        }

        return AUBase::SetParameter (inID, inScope, inElement, inValue, inBufferOffsetInFrames);
    }

    //==============================================================================
    ComponentResult Version()                   { return JucePlugin_VersionCode; }
    bool SupportsTail()                         { return true; }
    Float64 GetTailTime()                       { return (JucePlugin_TailLengthSeconds); }
    Float64 GetSampleRate()                     
	{ 
		return GetOutput(0)->GetStreamFormat().mSampleRate; 
	}

    Float64 GetLatency()
    {
        jassert (GetSampleRate() > 0);

        if (GetSampleRate() <= 0)
            return 0.0;

        return juceFilter->getLatencySamples() / GetSampleRate();
    }

    //==============================================================================
   #if BUILD_AU_CARBON_UI
    int GetNumCustomUIComponents()              { return 1; }

    void GetUIComponentDescs (ComponentDescription* inDescArray)
    {
        inDescArray[0].componentType = kAudioUnitCarbonViewComponentType;
        inDescArray[0].componentSubType = JucePlugin_AUSubType;
        inDescArray[0].componentManufacturer = JucePlugin_AUManufacturerCode;
        inDescArray[0].componentFlags = 0;
        inDescArray[0].componentFlagsMask = 0;
    }
   #endif

    //==============================================================================
    bool getCurrentPosition (AudioPlayHead::CurrentPositionInfo& info)
    {
        info.timeSigNumerator = 0;
        info.timeSigDenominator = 0;
        info.timeInSeconds = 0;
        info.editOriginTime = 0;
        info.ppqPositionOfLastBarStart = 0;
        info.isPlaying = false;
        info.isRecording = false;

        switch (lastSMPTETime.mType)
        {
            case kSMPTETimeType24:          info.frameRate = AudioPlayHead::fps24; break;
            case kSMPTETimeType25:          info.frameRate = AudioPlayHead::fps25; break;
            case kSMPTETimeType30Drop:      info.frameRate = AudioPlayHead::fps30drop; break;
            case kSMPTETimeType30:          info.frameRate = AudioPlayHead::fps30; break;
            case kSMPTETimeType2997:        info.frameRate = AudioPlayHead::fps2997; break;
            case kSMPTETimeType2997Drop:    info.frameRate = AudioPlayHead::fps2997drop; break;
            //case kSMPTETimeType60:
            //case kSMPTETimeType5994:
            default:                        info.frameRate = AudioPlayHead::fpsUnknown; break;
        }

        if (CallHostBeatAndTempo (&info.ppqPosition, &info.bpm) != noErr)
        {
            info.ppqPosition = 0;
            info.bpm = 0;
        }

        UInt32 outDeltaSampleOffsetToNextBeat;
        double outCurrentMeasureDownBeat;
        float num;
        UInt32 den;

        if (CallHostMusicalTimeLocation (&outDeltaSampleOffsetToNextBeat, &num, &den,
                                         &outCurrentMeasureDownBeat) == noErr)
        {
            info.timeSigNumerator = (int) num;
            info.timeSigDenominator = den;
            info.ppqPositionOfLastBarStart = outCurrentMeasureDownBeat;
        }

        double outCurrentSampleInTimeLine, outCycleStartBeat, outCycleEndBeat;
        Boolean playing, playchanged, looping;

        if (CallHostTransportState (&playing,
                                    &playchanged,
                                    &outCurrentSampleInTimeLine,
                                    &looping,
                                    &outCycleStartBeat,
                                    &outCycleEndBeat) == noErr)
        {
            info.isPlaying = playing;
            info.timeInSeconds = outCurrentSampleInTimeLine / GetSampleRate();
        }

        return true;
    }

    void sendAUEvent (const AudioUnitEventType type, const int index)
    {
        if (AUEventListenerNotify != 0)
        {
            auEvent.mEventType = type;
            auEvent.mArgument.mParameter.mParameterID = (AudioUnitParameterID) index;
            AUEventListenerNotify (0, 0, &auEvent);
        }
    }

    void audioProcessorParameterChanged (AudioProcessor*, int index, float /*newValue*/)
    {
        sendAUEvent (kAudioUnitEvent_ParameterValueChange, index);
    }

    void audioProcessorParameterChangeGestureBegin (AudioProcessor*, int index)
    {
        sendAUEvent (kAudioUnitEvent_BeginParameterChangeGesture, index);
    }

    void audioProcessorParameterChangeGestureEnd (AudioProcessor*, int index)
    {
        sendAUEvent (kAudioUnitEvent_EndParameterChangeGesture, index);
    }

    void audioProcessorChanged (AudioProcessor*)
    {
        // xxx is there an AU equivalent?
    }

    bool StreamFormatWritable (AudioUnitScope, AudioUnitElement)
    {
        return ! IsInitialized();
    }

    // (these two slightly different versions are because the definition changed between 10.4 and 10.5)
    ComponentResult StartNote (MusicDeviceInstrumentID, MusicDeviceGroupID, NoteInstanceID&, UInt32, const MusicDeviceNoteParams&) { return noErr; }
    ComponentResult StartNote (MusicDeviceInstrumentID, MusicDeviceGroupID, NoteInstanceID*, UInt32, const MusicDeviceNoteParams&) { return noErr; }
    ComponentResult StopNote (MusicDeviceGroupID, NoteInstanceID, UInt32)   { return noErr; }

    //==============================================================================
    ComponentResult Initialize()
    {
       #if ! JucePlugin_IsSynth
        const int numIns = GetInput(0) != 0 ? GetInput(0)->GetStreamFormat().mChannelsPerFrame : 0;
       #endif
        const int numOuts = GetOutput(0) != 0 ? GetOutput(0)->GetStreamFormat().mChannelsPerFrame : 0;

        bool isValidChannelConfig = false;

        for (int i = 0; i < numChannelConfigs; ++i)
          #if JucePlugin_IsSynth
            if (numOuts == channelConfigs[i][1])
          #else
            if (numIns == channelConfigs[i][0] && numOuts == channelConfigs[i][1])
          #endif
                isValidChannelConfig = true;

        if (! isValidChannelConfig)
            return kAudioUnitErr_FormatNotSupported;

        JuceAUBaseClass::Initialize();
        prepareToPlay();
        return noErr;
    }

    void Cleanup()
    {
        JuceAUBaseClass::Cleanup();

        if (juceFilter != nullptr)
            juceFilter->releaseResources();

        bufferSpace.setSize (2, 16);
        midiEvents.clear();
        incomingEvents.clear();
        prepared = false;
    }

    ComponentResult Reset (AudioUnitScope inScope, AudioUnitElement inElement)
    {
        if (! prepared)
            prepareToPlay();

        if (juceFilter != nullptr)
            juceFilter->reset();

        return JuceAUBaseClass::Reset (inScope, inElement);
    }

    void prepareToPlay()
    {
        if (juceFilter != nullptr)
        {
            juceFilter->setPlayConfigDetails (
                 #if ! JucePlugin_IsSynth
                  GetInput(0)->GetStreamFormat().mChannelsPerFrame,
                 #else
                  0,
                 #endif
                  GetOutput(0)->GetStreamFormat().mChannelsPerFrame,
                  GetSampleRate(),
                  GetMaxFramesPerSlice());

            bufferSpace.setSize (juceFilter->getNumInputChannels() + juceFilter->getNumOutputChannels(),
                                 GetMaxFramesPerSlice() + 32);

            juceFilter->prepareToPlay (GetSampleRate(), GetMaxFramesPerSlice());

            midiEvents.ensureSize (4096);
            midiEvents.clear();
            incomingEvents.ensureSize (4096);
            incomingEvents.clear();

            channels.calloc (jmax (juceFilter->getNumInputChannels(),
                                   juceFilter->getNumOutputChannels()) + 4);

            prepared = true;
        }
    }

    ComponentResult Render (AudioUnitRenderActionFlags &ioActionFlags,
                            const AudioTimeStamp& inTimeStamp,
                            UInt32 nFrames)
    {
        lastSMPTETime = inTimeStamp.mSMPTETime;

       #if ! JucePlugin_IsSynth
        return JuceAUBaseClass::Render (ioActionFlags, inTimeStamp, nFrames);
       #else
        // synths can't have any inputs..
        AudioBufferList inBuffer;
        inBuffer.mNumberBuffers = 0;

        return ProcessBufferLists (ioActionFlags, inBuffer, GetOutput(0)->GetBufferList(), nFrames);
       #endif
    }

    OSStatus ProcessBufferLists (AudioUnitRenderActionFlags& ioActionFlags,
                                 const AudioBufferList& inBuffer,
                                 AudioBufferList& outBuffer,
                                 UInt32 numSamples)
    {
        if (juceFilter != nullptr)
        {
            jassert (prepared);

            int numOutChans = 0;
            int nextSpareBufferChan = 0;
            bool needToReinterleave = false;
            const int numIn = juceFilter->getNumInputChannels();
            const int numOut = juceFilter->getNumOutputChannels();

            unsigned int i;
            for (i = 0; i < outBuffer.mNumberBuffers; ++i)
            {
                AudioBuffer& buf = outBuffer.mBuffers[i];

                if (buf.mNumberChannels == 1)
                {
                    channels [numOutChans++] = (float*) buf.mData;
                }
                else
                {
                    needToReinterleave = true;

                    for (unsigned int subChan = 0; subChan < buf.mNumberChannels && numOutChans < numOut; ++subChan)
                        channels [numOutChans++] = bufferSpace.getSampleData (nextSpareBufferChan++);
                }

                if (numOutChans >= numOut)
                    break;
            }

            int numInChans = 0;

            for (i = 0; i < inBuffer.mNumberBuffers; ++i)
            {
                const AudioBuffer& buf = inBuffer.mBuffers[i];

                if (buf.mNumberChannels == 1)
                {
                    if (numInChans < numOutChans)
                        memcpy (channels [numInChans], (const float*) buf.mData, sizeof (float) * numSamples);
                    else
                        channels [numInChans] = (float*) buf.mData;

                    ++numInChans;
                }
                else
                {
                    // need to de-interleave..
                    for (unsigned int subChan = 0; subChan < buf.mNumberChannels && numInChans < numIn; ++subChan)
                    {
                        float* dest;

                        if (numInChans < numOutChans)
                        {
                            dest = channels [numInChans++];
                        }
                        else
                        {
                            dest = bufferSpace.getSampleData (nextSpareBufferChan++);
                            channels [numInChans++] = dest;
                        }

                        const float* src = ((const float*) buf.mData) + subChan;

                        for (int j = numSamples; --j >= 0;)
                        {
                            *dest++ = *src;
                            src += buf.mNumberChannels;
                        }
                    }
                }

                if (numInChans >= numIn)
                    break;
            }

            {
                const ScopedLock sl (incomingMidiLock);
                midiEvents.clear();
                incomingEvents.swapWith (midiEvents);
            }

            {
                AudioSampleBuffer buffer (channels, jmax (numIn, numOut), numSamples);

                const ScopedLock sl (juceFilter->getCallbackLock());

                if (juceFilter->isSuspended())
                {
                    for (int i = 0; i < numOut; ++i)
                        zeromem (channels [i], sizeof (float) * numSamples);
                }
                else
                {
                    juceFilter->processBlock (buffer, midiEvents);
                }
            }

            if (! midiEvents.isEmpty())
            {
               #if JucePlugin_ProducesMidiOutput
                const JUCE_NAMESPACE::uint8* midiEventData;
                int midiEventSize, midiEventPosition;
                MidiBuffer::Iterator i (midiEvents);

                while (i.getNextEvent (midiEventData, midiEventSize, midiEventPosition))
                {
                    jassert (isPositiveAndBelow (midiEventPosition, (int) numSamples));



                    //xxx
                }
               #else
                // if your plugin creates midi messages, you'll need to set
                // the JucePlugin_ProducesMidiOutput macro to 1 in your
                // JucePluginCharacteristics.h file
                //jassert (midiEvents.getNumEvents() <= numMidiEventsComingIn);
               #endif

                midiEvents.clear();
            }

            if (needToReinterleave)
            {
                nextSpareBufferChan = 0;

                for (i = 0; i < outBuffer.mNumberBuffers; ++i)
                {
                    AudioBuffer& buf = outBuffer.mBuffers[i];

                    if (buf.mNumberChannels > 1)
                    {
                        for (unsigned int subChan = 0; subChan < buf.mNumberChannels; ++subChan)
                        {
                            const float* src = bufferSpace.getSampleData (nextSpareBufferChan++);
                            float* dest = ((float*) buf.mData) + subChan;

                            for (int j = numSamples; --j >= 0;)
                            {
                                *dest = *src++;
                                dest += buf.mNumberChannels;
                            }
                        }
                    }
                }
            }

           #if ! JucePlugin_SilenceInProducesSilenceOut
            ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;
           #endif
        }

        return noErr;
    }

protected:
    OSStatus HandleMidiEvent (UInt8 nStatus, UInt8 inChannel, UInt8 inData1, UInt8 inData2,
                             #if defined (MAC_OS_X_VERSION_10_5)
                              UInt32 inStartFrame)
                             #else
                              long inStartFrame)
                             #endif
    {
       #if JucePlugin_WantsMidiInput
        const ScopedLock sl (incomingMidiLock);
        const JUCE_NAMESPACE::uint8 data[] = { (JUCE_NAMESPACE::uint8) (nStatus | inChannel),
                                               (JUCE_NAMESPACE::uint8) inData1,
                                               (JUCE_NAMESPACE::uint8) inData2 };

        incomingEvents.addEvent (data, 3, inStartFrame);
       #endif

        return noErr;
    }

    OSStatus HandleSysEx (const UInt8* inData, UInt32 inLength)
    {
       #if JucePlugin_WantsMidiInput
        const ScopedLock sl (incomingMidiLock);
        incomingEvents.addEvent (inData, inLength, 0);
       #endif
        return noErr;
    }

    //==============================================================================
    ComponentResult GetPresets (CFArrayRef* outData) const
    {
        if (outData != nullptr)
        {
            const int numPrograms = juceFilter->getNumPrograms();
            presetsArray.ensureSize (sizeof (AUPreset) * numPrograms, true);
            AUPreset* const presets = (AUPreset*) presetsArray.getData();

            CFMutableArrayRef presetsArray = CFArrayCreateMutable (0, numPrograms, 0);

            for (int i = 0; i < numPrograms; ++i)
            {
                presets[i].presetNumber = i;
                presets[i].presetName = juceFilter->getProgramName (i).toCFString();

                CFArrayAppendValue (presetsArray, presets + i);
            }

            *outData = (CFArrayRef) presetsArray;
        }

        return noErr;
    }

    OSStatus NewFactoryPresetSet (const AUPreset& inNewFactoryPreset)
    {
        const int numPrograms = juceFilter->getNumPrograms();
        const SInt32 chosenPresetNumber = (int) inNewFactoryPreset.presetNumber;

        if (chosenPresetNumber >= numPrograms)
            return kAudioUnitErr_InvalidProperty;

        AUPreset chosenPreset;
        chosenPreset.presetNumber = chosenPresetNumber;
        chosenPreset.presetName = juceFilter->getProgramName(chosenPresetNumber).toCFString();

        juceFilter->setCurrentProgram (chosenPresetNumber);
        SetAFactoryPresetAsCurrent (chosenPreset);

        return noErr;
    }

    void componentMovedOrResized (Component& component, bool /*wasMoved*/, bool /*wasResized*/)
    {
        NSView* view = (NSView*) component.getWindowHandle();
        NSRect r = [[view superview] frame];
        r.origin.y = r.origin.y + r.size.height - component.getHeight();
        r.size.width = component.getWidth();
        r.size.height = component.getHeight();
        [[view superview] setFrame: r];
        [view setFrame: NSMakeRect (0, 0, component.getWidth(), component.getHeight())];
        [view setNeedsDisplay: YES];
    }

private:
    //==============================================================================
    ScopedPointer<MLPluginProcessor> juceFilter; // ML
    AudioSampleBuffer bufferSpace;
    HeapBlock <float*> channels;
    MidiBuffer midiEvents, incomingEvents;
    bool prepared;
    SMPTETime lastSMPTETime;
    AUChannelInfo channelInfo [numChannelConfigs];
    AudioUnitEvent auEvent;
    mutable juce::MemoryBlock presetsArray;
    CriticalSection incomingMidiLock;

    JUCE_DECLARE_NON_COPYABLE (JuceAU);
};

//==============================================================================
class EditorCompHolder  : public Component
{
public:
    EditorCompHolder (AudioProcessorEditor* const editor)
    {
 
		setSize (editor->getWidth(), editor->getHeight());
        addAndMakeVisible (editor);
		
		setOpaque(false); 

       #if ! JucePlugin_EditorRequiresKeyboardFocus
        setWantsKeyboardFocus (false);
		setMouseClickGrabsKeyboardFocus(false);
       #else
        setWantsKeyboardFocus (true);
       #endif
    }

    ~EditorCompHolder()
    {
        deleteAllChildren(); // note that we can't use a ScopedPointer because the editor may
                             // have been transferred to another parent which takes over ownership.
    }

    void childBoundsChanged (Component*)
    {
        Component* editor = getChildComponent(0);

        if (editor != nullptr)
        {
            const int w = jmax (32, editor->getWidth());
            const int h = jmax (32, editor->getHeight());

            if (getWidth() != w || getHeight() != h)
                setSize (w, h);

            NSView* view = (NSView*) getWindowHandle();
            NSRect r = [[view superview] frame];
            r.size.width = editor->getWidth();
            r.size.height = editor->getHeight();
            [[view superview] setFrame: r];
            [view setFrame: NSMakeRect (0, 0, editor->getWidth(), editor->getHeight())];
            [view setNeedsDisplay: YES];
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE (EditorCompHolder);
};

//==============================================================================
@implementation JuceUIViewClass

- (JuceUIViewClass*) initWithFilter: (AudioProcessor*) filter_
                             withAU: (JuceAU*) au_
                      withComponent: (AudioProcessorEditor*) editorComp_
{
    filter = filter_;
    au = au_;
    editorComp = new EditorCompHolder (editorComp_);

    [super initWithFrame: NSMakeRect (0, 0, editorComp_->getWidth(), editorComp_->getHeight())];
    [self setHidden: NO];
    [self setPostsFrameChangedNotifications: YES];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector (applicationWillTerminate:)
                                                 name: NSApplicationWillTerminateNotification
                                               object: nil];
    activeUIs.add (self);

    editorComp->setOpaque (false);
   #if JucePlugin_EditorRequiresKeyboardFocus
    editorComp->addToDesktop (0, (void*)self);
    editorComp->setWantsKeyboardFocus (true);
   #else
    editorComp->addToDesktop (ComponentPeer::windowIgnoresKeyPresses, (void*)self);
    editorComp->setWantsKeyboardFocus (false);
   #endif
    
    editorComp->setVisible (true);

    return self;
}

- (BOOL) acceptsFirstResponder
{
    BOOL ret;
    #if JucePlugin_EditorRequiresKeyboardFocus
    ret = YES;
    #else
    ret = NO;
    #endif
    return ret;
}
- (BOOL) becomeFirstResponder
{
    BOOL ret = [self acceptsFirstResponder];
    return ret;
}
- (BOOL) resignFirstResponder
{
    BOOL ret = [self acceptsFirstResponder];
    return ret;
}

- (BOOL) canBecomeKeyWindow
{    
    BOOL ret = [self acceptsFirstResponder];
    return ret;
}

- (BOOL) acceptsFirstMouse: (NSEvent*) ev
{
    return YES;
}

- (void) keyDown: (NSEvent*) ev
{
    [[self window] makeFirstResponder: self];
    [super keyDown: ev];
    // the esc key can delete the editor so check if editorComp still exists and if it does set the focus back to it
    if (editorComp)
    {
        NSView* view = (NSView*)(editorComp->getPeer()->getNativeHandle());
        [[self window] makeFirstResponder: view];
    }
}

- (void) keyUp: (NSEvent*) ev
{
    [[self window] makeFirstResponder: self];
    [super keyUp: ev];
    // the esc key can delete the editor so check if editorComp still exists and if it does set the focus back to it
    if (editorComp)
    {
        NSView* view = (NSView*)(editorComp->getPeer()->getNativeHandle());
        [[self window] makeFirstResponder: view];
    }
}

/*
//==============================================================================
@implementation JuceUIViewClass

- (JuceUIViewClass*) initWithFilter: (AudioProcessor*) filter_
                             withAU: (JuceAU*) au_
                      withComponent: (AudioProcessorEditor*) editorComp_
{
    filter = filter_;
    au = au_;
    editorComp = new EditorCompHolder (editorComp_);

    [super initWithFrame: NSMakeRect (0, 0, editorComp_->getWidth(), editorComp_->getHeight())];
    [self setHidden: NO];
    [self setPostsFrameChangedNotifications: YES];

    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector (applicationWillTerminate:)
                                                 name: NSApplicationWillTerminateNotification
                                               object: nil];
    activeUIs.add (self);

    editorComp->addToDesktop (0, (void*) self);
    editorComp->setVisible (true);

    return self;
}
*/

- (void) dealloc
{
    if (activeUIs.contains (self))
        [self shutdown];

    [super dealloc];
}

- (void) applicationWillTerminate: (NSNotification*) aNotification
{
    (void) aNotification;
    [self shutdown];
}

- (void) shutdown
{
    // there's some kind of component currently modal, but the host
    // is trying to delete our plugin..
    jassert (Component::getCurrentlyModalComponent() == nullptr);

    [[NSNotificationCenter defaultCenter] removeObserver: self];
    [self deleteEditor];

    jassert (activeUIs.contains (self));
    activeUIs.removeFirstMatchingValue (self);
    if (activePlugins.size() + activeUIs.size() == 0)
        shutdownJuce_GUI();
}

- (void) viewDidMoveToWindow
{
    if ([self window] != nil)
    {
        [[self window] setAcceptsMouseMovedEvents: YES];

        if (editorComp != nullptr)
            [[self window] makeFirstResponder: (NSView*) editorComp->getWindowHandle()];
    }
}

- (BOOL) mouseDownCanMoveWindow
{
    return NO;
}

- (void) deleteEditor
{
    if (editorComp != nullptr)
    {
        if (editorComp->getChildComponent(0) != nullptr)
            if (activePlugins.contains ((void*) au)) // plugin may have been deleted before the UI
                filter->editorBeingDeleted ((AudioProcessorEditor*) editorComp->getChildComponent(0));

        deleteAndZero (editorComp);
    }

    editorComp = nullptr;
}

- (void) filterBeingDeleted: (JuceAU*) au_
{
    if (au_ == au)
        [self deleteEditor];
}

@end

//==============================================================================
@implementation JuceUICreationClass

- (JuceUICreationClass*) init
{
    return [super init];
}

- (void) dealloc
{
    [super dealloc];
}

- (unsigned) interfaceVersion
{
    return 0;
}

- (NSString*) description
{
    return [NSString stringWithString: @JucePlugin_Name];
}

- (NSView*) uiViewForAudioUnit: (AudioUnit) inAudioUnit
                      withSize: (NSSize) inPreferredSize
{
    void* pointers[2];
    UInt32 propertySize = sizeof (pointers);

    if (AudioUnitGetProperty (inAudioUnit,
                              juceFilterObjectPropertyID,
                              kAudioUnitScope_Global,
                              0,
                              pointers,
                              &propertySize) != noErr)
        return nil;

    AudioProcessor* filter = (AudioProcessor*) pointers[0];
    JuceAU* au = (JuceAU*) pointers[1];

    if (filter == nullptr)
        return nil;

    AudioProcessorEditor* editorComp = filter->createEditorIfNeeded();

    if (editorComp == nullptr)
        return nil;
		
	// ML
	MLPluginEditor* mlEditor = static_cast<MLPluginEditor*> (editorComp);
	mlEditor->setWrapperFormat(MLPluginFormats::eAUPlugin);
	// ML

    return [[[JuceUIViewClass alloc] initWithFilter: filter
                                             withAU: au
                                      withComponent: editorComp] autorelease];
}
@end



//==============================================================================
#define JUCE_COMPONENT_ENTRYX(Class, Name, Suffix) \
extern "C" __attribute__((visibility("default"))) ComponentResult Name ## Suffix (ComponentParameters* params, Class* obj); \
extern "C" __attribute__((visibility("default"))) ComponentResult Name ## Suffix (ComponentParameters* params, Class* obj) \
{ \
    return ComponentEntryPoint<Class>::Dispatch(params, obj); \
}

#define JUCE_COMPONENT_ENTRY(Class, Name, Suffix) JUCE_COMPONENT_ENTRYX(Class, Name, Suffix)

JUCE_COMPONENT_ENTRY (JuceAU, JucePlugin_AUExportPrefix, Entry)

#if BUILD_AU_CARBON_UI
  JUCE_COMPONENT_ENTRY (JuceAUView, JucePlugin_AUExportPrefix, ViewEntry)
#endif

#endif
