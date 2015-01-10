#!/bin/bash
echo "building proto for net"
pushd proto
for file in `ls *.proto`;
do
    files=$files' -i:'$file
done

mono ../deps/protobuf-net/ProtoGen/bin/Debug/protogen.exe $files -o:../net/maid/maid/library/ControllerMeta.cs -p:xml
mcs -t:library ../net/maid/maid/library/ControllerMeta.cs -reference:../net/maid/maid/library/protobuf-net.dll -sdk:2
mono ../deps/protobuf-net/precompile/bin/Debug/precompile.exe ../net/maid/maid/library/ControllerMeta.dll -t:maid.proto.ControllerMetaSerializer -o:../net/maid/maid/library/ControllerMetaSerializer.dll
popd
