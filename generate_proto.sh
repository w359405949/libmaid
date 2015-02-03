#!/bin/bash
echo "generate proto for cpp"
pushd include
protoc --cpp_out=. --python_out=. --proto_path=. --proto_path=$HOME/.local/include maid/*.proto
popd

mv include/maid/*.pb.cc src/
mv include/maid/*.py python/maid/
echo "done"

echo "generate proto for test"
pushd test
protoc --cpp_out=. *.proto
popd
echo "done"
