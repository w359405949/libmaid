mono ProtoGen/bin/Debug/protogen.exe -i:maid/example/proto/hello.proto -o:maid/example/proto/Hello.cs
mcs -t:library maid/example/proto/Hello.cs -reference:maid/example/proto/protobuf-net.dll -sdk:2
mono precompile/bin/Debug/precompile.exe maid/example/proto/Hello.dll -t:maid.example.HelloSerializer -o:maid/example/proto/HelloSerializer.dll
