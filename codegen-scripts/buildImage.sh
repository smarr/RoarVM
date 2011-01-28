# set $SQUEAKVM env var to path to VM binary
# for example: 
# SQUEAKVM=/home/sig/vm/bin/squeak; sh ./buildImage.sh

rm -rf ../build
mkdir -p ../build/Image
cp ./image.url ../build/Image
cp ./LoadVMMaker.st ../build/Image
cd ../build/Image || exit

wget --no-check-certificate -i ./image.url -O ./image.zip
unzip -jbo ./image.zip
$SQUEAKVM -headless `ls *.image` LoadVMMaker.st

