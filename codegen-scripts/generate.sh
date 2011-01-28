# script for generating the sources and config
# pass the script name as argument
# set $SQUEAKVM env var to path to VM binary
# for example: 
# SQUEAKVM=/home/sig/vm/bin/squeak; sh ./generate.sh ./CogUnix.st

cp "$1" ../build/Image || exit
cd ../build/Image || exit

$SQUEAKVM -headless generator.image "$1"

