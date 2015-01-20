#!/bin/bash
echo "generate proto for example"

cp net/maid/maid/library/protobuf-net.dll net/maid/example/library/
ln -s $HOME/.local/include/maid .
ln -s $HOME/.local/include/google .

mono deps/protobuf-net/ProtoGen/bin/Debug/protogen.exe -i:example/hello/hello.proto -o:net/maid/example/library/Hello.cs
mcs -t:library net/maid/example/library/Hello.cs -reference:net/maid/example/library/protobuf-net.dll -sdk:2
mono deps/protobuf-net/precompile/bin/Debug/precompile.exe net/maid/example/library/Hello.dll -t:maid.example.HelloSerializer -o:net/maid/example/library/HelloSerializer.dll

rm maid
rm google
