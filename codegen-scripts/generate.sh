# script for generating the sources and build config
# pass the configuration class name as argument
# The $SQUEAKVM env var should be set to path to VM binary
# for example: 
#
# export SQUEAKVM=/home/sig/vm/bin/squeak
#
# sh ./generate.sh -headless CogUnixConfig
#

rm -rf ../src  
cd ../build || exit
rm PharoDebug.log

echo "$2 generateWithSources. Smalltalk snapshot: false andQuit: true." > ./script.st

$SQUEAKVM "$1" generator.image script.st

