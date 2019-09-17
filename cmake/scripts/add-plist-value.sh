#!/bin/sh

# add-plist-value
#
# This script takes 3 arguments: 
# - key string
# - value string
# - file name
# 
KEY_STRING="$1"
VALUE_STRING="$2"
FILE_NAME="$3"

DICT_END="<\/dict>"
LINE1="<key>"$KEY_STRING"</key>"
LINE2="\t<string>"$VALUE_STRING"</string>"

echo $KEY_STRING
echo $VALUE_STRING

if
! grep -q $KEY_STRING $FILE_NAME
then
	echo 'fixing info.plist...'
	perl -i -pe 's/'$DICT_END'/\t<key>'$KEY_STRING'<\/key>\n\t<string>'"$VALUE_STRING"'<\/string>\n'$DICT_END'/;'  $FILE_NAME
fi