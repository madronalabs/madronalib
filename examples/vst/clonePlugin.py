#!/usr/bin/env python3

import shutil
import sys
import os
import string
import setPluginInfo

# count the arguments
arguments = len(sys.argv) - 1


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

	setPluginInfo.replaceAttrInFiles("name", pluginName)


# allow running from cmd line
if __name__ == "__main__":
    
	# count the arguments
	arguments = len(sys.argv) - 1
	if arguments != 2:
		print("usage: clonePlugin <destination> <pluginName>")
		quit()
	clonePlugin(sys.argv[1], sys.argv[2])
