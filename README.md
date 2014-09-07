libmaid
=======

a general use of rpc service, implement by libev &amp; protobuf

proto format
======
basic format:|    4 bytes          |  4 bytes     |  n bytes     |n bytes|
description :|ControllerMeta length|Message length|ControllerMeta|Message|


Dependency
=======

libev, protobuf


Build
=======

cmake .
make

Build Example
=======

cp lib/libmaid.a example/hello/
cd example/hello/
g++ client.cpp libmaid.a echo.pb.cc -lev -lprotobuf -o client
g++ server.cpp libmaid.a echo.pb.cc -lev -lprotobuf -o server
./server
./client
