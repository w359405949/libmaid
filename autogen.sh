pushd proto
protoc *.proto --cpp_out=.
popd

for file in `find proto/ -name '*.pb.*'`
do
   mv $file src/
done

TOPDIR=`pwd`

libtoolize -f -c
aclocal
autoheader
autoconf
automake -a
mkdir build -p
pushd build
../configure --prefix=$TOPDIR
make
make install
popd
