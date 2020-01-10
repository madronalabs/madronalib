#!/usr/bin/env python3

import shutil
import sys
import os

targetFiles = ["source/au/audiounitconfig.h"]#, "source/au/Info.plist", "llllpluginnamellll.rc", "source/version.h",
#"source/factoryDefinition.cpp", "CMakeLists.txt"]


def set(attr, value):

	cwd = os.getcwd()
	print ("wd:", cwd)

	print (cwd + "/test")

	attr = (sys.argv[1])
	value = (sys.argv[2])

	print (attr, "->", value)

	validAttr = True

	if(attr == 'name'):
		searchText = 'llllPluginNamellll'
	elif(attr == 'company'):
		print('hey')
	elif(attr == 'subtype'):
		print('hey')
	elif(attr == 'mfgr'):
		print('hey')
	elif(attr == 'url'):
		print('hey')
	elif(attr == 'email'):
		print('hey')
	else:
		validAttr = False

	replaceText = value
	
	print ("vaild? " , validAttr)


	# not all strings we want to replace will be in all these files,
	# but for simplicity we just grind through all the files anyway.
	if(validAttr):
		for file in targetFiles:
			fullName = cwd + "/" + file
			print(fullName)
			if os.path.exists(fullName):
				# search and replace, creating temp file
				input = open(fullName)
				tempTag = "__TEMP__"
				output = open(fullName + tempTag, 'w')
				output.write(input.read().replace(searchText, replaceText))
				input.close()
				output.close()
				# copy temp file back over original
				shutil.move(fullName + tempTag, fullName)


	return

# allow running from cmd line
if __name__ == "__main__":
	print("main")
    
	# count the arguments
	arguments = len(sys.argv) - 1

	if arguments != 2:
		print("usage: setPluginInfo <attribute> <stringValue>")
		quit()
	#set("a", "b")
	set(sys.argv[1], sys.argv[2])
