if [ -f libfreetype.a ]
then
exit 0
fi

if [ ! -d freetype2 ]
then
  /usr/local/git/bin/git clone git://git.sv.gnu.org/freetype/freetype2.git
fi

cd freetype2
/usr/local/git/bin/git checkout VER-2-4-4

/bin/sh autogen.sh
./configure CFLAGS="-arch i386" LDFLAGS="-arch i386" --without-zlib

/usr/bin/make clean
/usr/bin/make 2>1 > ../build.log
cd ..
cp ./freetype2/objs/.libs/libfreetype.a ./


