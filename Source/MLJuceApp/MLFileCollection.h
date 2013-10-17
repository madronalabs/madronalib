//
//  MLFileCollection.h
//  Soundplane
//
//  Created by Randy Jones on 10/10/13.
//
//

#ifndef __Soundplane__MLFileCollection__
#define __Soundplane__MLFileCollection__

#include "JuceHeader.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"

// a collection of files matching some kind of critertia.
// TODO just moving the existing plugin code into here for now.
// not implementing the callback or background stuff.
class MLFileCollection :
    public Timer
{
public:
    
    // a Listener class must have a processFile routine to do something
	// with each file as it is found.
    class Listener
	{
	public:
		virtual ~Listener() {}
		virtual void processFile (const MLSymbol collection, const File& f, int idx) = 0;
	};
	
    class FileInfo
    {
    public:
        FileInfo(File f, String p, String n) : mFile(f), mRelativePath(p), mShortName(n){}
        ~FileInfo(){}
        File mFile;
        String mRelativePath;
        String mShortName;
    };
    
 	MLFileCollection(MLSymbol name, const File startDir, String extension);
    ~MLFileCollection();
    
    void setListener (Listener* listener);

    // TODO look for files asynchronously
    void timerCallback();
    
    // find all files matching description in the start dir.
    // returns the number of files found.
    int findFilesImmediate();
   
    //int getFileIndexByPath(String path);
    const File& getFileByIndex(int idx);
    
    MLMenuPtr buildMenu(bool flat = false);
    void dump();
private:
    MLSymbol mName;
	const File mStartDir;
    String mExtension;
    Listener* mpListener;
    std::vector <FileInfo> mFiles;
};

typedef std::tr1::shared_ptr<MLFileCollection> MLFileCollectionPtr;

#endif /* defined(__Soundplane__MLFileCollection__) */
