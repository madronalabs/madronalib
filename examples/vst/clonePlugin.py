#!/usr/bin/env python3

import shutil
import sys
import os
import string
import setPluginInfo
import uuid

# count the arguments
arguments = len(sys.argv) - 1

def genFUID():
	x = uuid.uuid4().hex
	outStr = ''
	for i in range(4):
		subStr = x[i*8:(i + 1)*8]
		outStr = outStr + '0x' + subStr
		if i < 3:
			outStr = outStr + ', '
	return outStr



def clonePlugin(dest, pluginName):
	pluginNameLC = str.lower(pluginName)
	newPlugDest = dest + "/" + pluginNameLC

	cwd = os.getcwd()
	print ("new project destination:", newPlugDest)

	try:
		shutil.copytree(cwd, newPlugDest)
	except:
		print("error copying directory tree to ", newPlugDest)
		return

	os.chdir(newPlugDest)	
	cwd = os.getcwd()

	# remove build directory if one exists
	dirpath = os.path.join(cwd, 'build')
	if os.path.exists(dirpath) and os.path.isdir(dirpath):
	    shutil.rmtree(dirpath)

	setPluginInfo.replaceAttrInFiles("uida", genFUID())
	setPluginInfo.replaceAttrInFiles("uidb", genFUID())
	setPluginInfo.replaceAttrInFiles("name", pluginName)



# allow running from cmd line
if __name__ == "__main__":
    
	# count the arguments
	arguments = len(sys.argv) - 1
	if arguments < 2:
		print("usage: clonePlugin destination pluginName (company) (mfgr) (subtype) (url) (email)")
		quit()
	clonePlugin(sys.argv[1], sys.argv[2])
	if arguments >= 3:
		setPluginInfo.replaceAttrInFiles("company", sys.argv[3])
	if arguments >= 4:
		setPluginInfo.replaceAttrInFiles("mfgr", sys.argv[4])
	if arguments >= 5:
		setPluginInfo.replaceAttrInFiles("subtype", sys.argv[5])
	if arguments >= 6:
		setPluginInfo.replaceAttrInFiles("url", sys.argv[6])
	if arguments >= 7:
		setPluginInfo.replaceAttrInFiles("email", sys.argv[7])
