#!/bin/bash
echo "generate proto for net"
pushd include
for file in `find maid/*.proto`;
do
    files=$files' -i:'$file
done

ln -s $HOME/.local/include/google .

mono ../deps/protobuf-net/ProtoGen/bin/Debug/protogen.exe $files -o:../net/maid/maid/library/ControllerProto.cs
mcs -t:library ../net/maid/maid/library/ControllerProto.cs -reference:../net/maid/maid/library/protobuf-net.dll -sdk:2
mono ../deps/protobuf-net/precompile/bin/Debug/precompile.exe ../net/maid/maid/library/ControllerProto.dll -t:maid.proto.ControllerProtoSerializer -o:../net/maid/maid/library/ControllerProtoSerializer.dll

rm google
popd
