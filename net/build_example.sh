mono ProtoGen/bin/Debug/protogen.exe -i:example/hello.proto -o:example/Hello.cs
mcs -t:library example/Hello.cs -reference:example/protobuf-net.dll
mono precompile/bin/Debug/precompile.exe example/Hello.dll -t:maid.example.HelloSerializer -o:example/HelloSerializer.dll
