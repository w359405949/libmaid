#!/bin/bash
echo "building proto for python, cpp"
export PATH=`pwd`/deps/protobuf/src:$PATH
export LD_LIBRARY_PATH=`pwd`/deps/protobuf/src/.libs/
pushd proto
protoc --cpp_out=. --python_out=. *
popd
mv proto/*.h src/include/
mv proto/*.cc src/
mv proto/*.py python/maid/
echo "done"
echo

#echo "building proto for net"
#mono net/ProtoGen/bin/Debug/protogen.exe -i:proto/controller.proto -o:net/maid/maid/library/ControllerMeta.cs
#mcs -t:library net/maid/maid/library/ControllerMeta.cs -reference:net/maid/maid/library/protobuf-net.dll -sdk:2
#mono net/precompile/bin/Debug/precompile.exe net/maid/maid/library/ControllerMeta.dll -t:maid.proto.ControllerMetaSerializer -o:net/maid/maid/library/ControllerMetaSerializer.dll
