# set $SQUEAKVM env var to path to VM binary
# for example: 
# export SQUEAKVM=/home/sig/vm/bin/squeak; sh ./buildImage.sh -headless 

rm -rf ../src
rm -rf ../build
mkdir -p ../build
cp ./image.url ../build
cp ./LoadVMMaker.st ../build
cd ../build || exit

wget --no-check-certificate -i ./image.url -O ./image.zip
unzip -jbo ./image.zip
IMAGE=`ls *.image`
$SQUEAKVM "$1" $IMAGE ./LoadVMMaker.st

