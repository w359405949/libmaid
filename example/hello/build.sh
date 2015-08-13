protoc --cpp_out=. *.proto --proto_path=$HOME/.local/include/ --proto_path=.

g++ -ggdb -O0 -std=c++11 -I../../include -I../../src -I/usr/local/include -I../../deps/libuv/include -I../../deps/protobuf/src client.cc *.pb.cc ../../libmaid.a ../../deps/libuv/.libs/libuv.a ../../deps/protobuf/src/.libs/libprotobuf.a ../../deps/glog/libglog.a -o client
g++ -ggdb -O0 -std=c++11 -I../../include -I../../src -I/usr/local/include -I../../deps/libuv/include -I../../deps/protobuf/src server.cc *.pb.cc ../../libmaid.a ../../deps/libuv/.libs/libuv.a ../../deps/protobuf/src/.libs/libprotobuf.a ../../deps/glog/libglog.a -o server
