libmaid
=======

a general use of rpc service, implement by libev &amp; protobuf


Dependency
=======

libev, protobuf


Build
=======
./autogen.sh

Build Example
=======
cp lib/libmaid.a example/hello/
cd example/hello/
g++ client.cpp libmaid.a -lev -lprotobuf -o client
g++ server.cpp libmaid.a -lev -lprotobuf -o server
./server
./client
