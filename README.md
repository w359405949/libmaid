libmaid
=======

a simple, light weight, general use and multiple language(c++, python, c#-unity3d) support of protobuf rpc service implement.

dependency
=======

c++: libuv, protobuf

python: gevent, protobuf

c#: .net2.0, protobuf-net

proto format
======

basic format:|----------4 bytes------------------|-----n bytes----|

description :|-ControllerMeta length(Big Endian)-|-ControllerMeta-|


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
