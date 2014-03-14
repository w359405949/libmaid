pushd proto
protoc controller.proto --cpp_out=.
popd

mv proto/controller.pb.cc src/
mv proto/controller.pb.h include/

TOPDIR=`pwd`
libtoolize -f -c
aclocal
autoheader
autoconf
automake -a
mkdir build -p
pushd build
../configure --prefix=$TOPDIR CXXFLAGS='-g -O0' CFLAGS='-g -O0'
make
make install
popd
