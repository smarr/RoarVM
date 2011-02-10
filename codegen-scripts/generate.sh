# script for generating the sources and config
# pass the script name as argument
# set $SQUEAKVM env var to path to VM binary
# for example: 
# export SQUEAKVM=/home/sig/vm/bin/squeak; sh ./generate.sh -headless ./CogUnix.st

cp "$2" ../build || exit
cd ../build || exit

$SQUEAKVM "$1" generator.image "$2"

