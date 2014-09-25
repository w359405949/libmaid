mono net/ProtoGen/bin/Debug/protogen.exe -i:proto/controller.proto -o:net/Plugins/Controller.cs
mcs -t:library net/Plugins/Controller.cs -reference:net/Plugins/protobuf-net.dll
mono net/precompile/bin/Debug/precompile.exe net/Plugins/Controller.dll -t:maid.proto.ControllerSerializer -o:net/Plugins/ControllerSerializer.dll
