protoc --cpp_out=. *.proto --proto_path=.

g++ -ggdb -O0 -std=c++11 -I../../include -I../../src -I/usr/local/include -I../../deps/libuv/include -I../../deps/protobuf/src client.cc *.pb.cc ../../libmaid.a ../../deps/libuv/.libs/libuv.a ../../deps/protobuf/libprotobuf.a -o client
g++ -ggdb -O0 -std=c++11 -I../../include -I../../src -I/usr/local/include -I../../deps/libuv/include -I../../deps/protobuf/src server.cc *.pb.cc ../../libmaid.a ../../deps/libuv/.libs/libuv.a ../../deps/protobuf/libprotobuf.a -o server
