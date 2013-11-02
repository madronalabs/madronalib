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
#include "MLFile.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"

// a collection of files matching some kind of critertia.
// TODO just moving the existing plugin code into here for now.
// planned timer / background scan stuff unimplemented.
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
    
 	MLFileCollection(MLSymbol name, const File startDir, String extension);
    ~MLFileCollection();
    void clear();
    int size() { return mFilesByIndex.size(); }
    
    void setListener (Listener* listener);

    // TODO look for files asynchronously
    void timerCallback();
    
    // find all files matching description in the start dir.
    // returns the number of files found.
    int findFilesImmediate();
   
    const MLFilePtr getFileByName(const std::string& name);
    
    MLMenuPtr buildMenu(bool flat = false);
    void dump();
    
private:
    // the file tree
    MLFile mRoot;
    std::vector<MLFilePtr> mFilesByIndex;    
    MLSymbol mName;
    String mExtension;
    Listener* mpListener;
};

typedef std::tr1::shared_ptr<MLFileCollection> MLFileCollectionPtr;

#endif /* defined(__Soundplane__MLFileCollection__) */
