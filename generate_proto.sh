#!/bin/bash
pushd include
echo "generate proto for cpp"
protoc --cpp_out=. --proto_path=. --proto_path=$HOME/.local/include maid/*.proto
popd

mv include/maid/*.pb.cc src/
#mv proto/*.py python/maid/
echo "done"
