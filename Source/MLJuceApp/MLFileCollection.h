//
//  MLFileCollection.h
//  Soundplane
//
//  Created by Randy Jones on 10/10/13.
//
//

#ifndef __MLFileCollection__
#define __MLFileCollection__

#include "JuceHeader.h"
#include "MLFile.h"
#include "MLDefaultFileLocations.h"
#include "MLMenu.h"

// a collection of files matching some kind of criteria.
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
   
    // return a file by its path + name relative to our starting directory.
    const MLFilePtr getFileByName(const std::string& name);

    std::string getFileNameByIndex(int idx);
    MLFilePtr getFileByIndex(int idx);
    const int getFileIndexByName(const std::string& fullName);

    // make a new file.
    const MLFilePtr createFile(const std::string& relativePath);

    // given a full system file path, get its path relative to our starting directory.
    std::string getRelativePath(const std::string& name);
    
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

#endif 
