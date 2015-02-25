
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#include "MLJuceFilesMac.h"

#if JUCE_MAC

juce::XmlElement* loadPropertyFileToXML(const juce::File& f) 
{
	juce::String juceName = f.getFullPathName();
	const char* fileStr = juceName.toUTF8();

	CFPropertyListRef propertyList;
	CFStringRef       errorString;
	CFDataRef         resourceData;
	// Boolean           status;
	SInt32            errorCode;
	juce::XmlElement* ret = nullptr;

	// get URL from Juce File
	CFStringRef fileNameString = CFStringCreateWithCString(NULL, fileStr, kCFStringEncodingUTF8);
	CFURLRef fileURL = CFURLCreateWithFileSystemPath(NULL, fileNameString, kCFURLPOSIXPathStyle, false);

	// Read the CFData file containing the encoded XML.
	CFURLCreateDataAndPropertiesFromResource(
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
			   
	CFDictionaryRef dict = (CFDictionaryRef) propertyList;
	CFDataRef pluginStateData = 0;
	
	// get saved preset name
	char nameBuf[33];
	CFStringRef nameString = CFStringCreateWithCString(NULL, "name", kCFStringEncodingUTF8);
	CFStringRef presetName = reinterpret_cast<CFStringRef>(CFDictionaryGetValue (dict, nameString));		
	const bool nameResult = (CFStringGetCString(presetName, nameBuf, 32, kCFStringEncodingASCII));
	
	if (CFDictionaryGetValueIfPresent (dict, CFSTR("jucePluginState"), (const void**) &pluginStateData))
	{
		if (pluginStateData != 0)
		{
			const int numBytes = (int) CFDataGetLength (pluginStateData);
			const void* rawBytes = CFDataGetBytePtr (pluginStateData);

			if (numBytes > 16)
			{
				const char* xmlBytes = static_cast<const char*> (rawBytes) + 8;
				const int xmlLength = numBytes - 8;
				
				// parse() allocates a new XmlElement
				juce::XmlElement* xmlState = juce::XmlDocument::parse (juce::String::fromUTF8 (xmlBytes, xmlLength));
				if (nameResult)
				{
					xmlState->setAttribute ("presetName", juce::String(nameBuf));	
				}

				ret = xmlState;
			}
		}
	}
	
	CFRelease(resourceData);
	CFRelease(propertyList);
	CFRelease(nameString);
    CFRelease(fileURL);
	CFRelease(fileNameString);
	return ret;
}


// defined in juce_Audioprocessor.cpp
//static const unsigned long magicXmlNumber = 0x21324356;

/*
void writeXMLToPropertyFile(XmlElement* xml, File& f) 
{
	String juceName = f.getFullPathName();
	const char* fileStr = juceName.toCString();
	String shortName = f.getFileNameWithoutExtension();

	// get URL from Juce File
	CFURLRef fileURL = CFURLCreateWithFileSystemPath(NULL, CFStringCreateWithCString(NULL, fileStr, kCFStringEncodingUTF8), kCFURLPOSIXPathStyle, false);

	// get XML in Juce format binary
	const String xmlString (xml->createDocument (String::empty, true, false));
    const int stringLength = xmlString.getNumBytesAsUTF8();
	MemoryBlock destData;
	destData.setSize (stringLength + 10);
    char* const d = static_cast<char*> (destData.getData());
    *(uint32*) d = ByteOrder::swapIfBigEndian ((const uint32) magicXmlNumber);
    *(uint32*) (d + 4) = ByteOrder::swapIfBigEndian ((const uint32) stringLength);
    xmlString.copyToUTF8 (d + 8, stringLength + 1);
	
	// create a property list. 
	CFPropertyListRef myPropsRef;



	ComponentResult err = JuceAUBaseClass::SaveState (&myPropsRef);
	if (err != noErr) return;

	jassert (CFGetTypeID (myPropsRef) == CFDictionaryGetTypeID());

	CFMutableDictionaryRef dict = (CFMutableDictionaryRef) myPropsRef;	
	
}

*/



#endif // JUCE_MAC
