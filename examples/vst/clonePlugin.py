import shutil
import sys
import os
import string
import setPluginInfo

# count the arguments
arguments = len(sys.argv) - 1

if arguments != 2:
	print("usage: clonePlugin <destination> <pluginName>")
	quit()

dest = (sys.argv[1])
pluginName = (sys.argv[2])

pluginNameLC = str.lower(pluginName)
newPlugDest = dest + "/" + pluginNameLC

cwd = os.getcwd()
print ("dest:", newPlugDest)

#shutil.copytree(cwd, newPlugDest)

print (setPluginInfo.set("a", "b"))
