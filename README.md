libmaid
=======

a general use of rpc service, support c++, python, c#(unity3d, ios)

dependency
=======

c++: libuv, protobuf
python: gevent, protobuf
c#: .net2.0, protobuf-net

proto format
======

basic format:|--------4 bytes------------------|---n bytes----|

description :|ControllerMeta length(Big Endian)|ControllerMeta---|


Build
=======

cmake .

make && make install

Build Example
=======

cp lib/libmaid.a example/hello/

cd example/hello/

g++ client.cpp libmaid.a echo.pb.cc -lev -lprotobuf -o client

g++ server.cpp libmaid.a echo.pb.cc -lev -lprotobuf -o server

./server

./client
