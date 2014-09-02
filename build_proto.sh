export PATH=`pwd`/deps/protobuf/src:$PATH
export LD_LIBRARY_PATH=`pwd`/deps/protobuf/src/.libs/
pushd proto
protoc --cpp_out=. --python_out=. *
popd
mv proto/*.h include/
mv proto/*.cc src/
mv proto/*.py python/maid/
