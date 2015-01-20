#!/bin/bash
pushd include
echo "generate proto for cpp"
protoc --cpp_out=. --python_out=. --proto_path=. --proto_path=$HOME/.local/include maid/*.proto
popd

mv include/maid/*.pb.cc src/
mv include/maid/*.py python/maid/
echo "done"
