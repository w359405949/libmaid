mono net/ProtoGen/bin/Debug/protogen.exe -i:proto/controller.proto -o:net/maid/maid/library/ControllerMeta.cs
mcs -t:library net/maid/maid/library/ControllerMeta.cs -reference:net/maid/maid/library/protobuf-net.dll -sdk:2
mono net/precompile/bin/Debug/precompile.exe net/maid/maid/library/ControllerMeta.dll -t:maid.proto.ControllerMetaSerializer -o:net/maid/maid/library/ControllerMetaSerializer.dll
