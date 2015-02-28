libmaid
=======

a simple, light weight, general use and multiple language(c++, python, c#-unity3d) support of protobuf rpc service implement.

dependency
=======

c++:

    libuv, protobuf


python:

    gevent, protobuf


c#:

    .net2.0, protobuf-net

proto format
======

basic format:|----------4 bytes------------------|-----n bytes----|

description :|-ControllerMeta length(Big Endian)-|-ControllerProto-|


Build && install
=======

c++:

    cmake .

    make && make install


    # unittest

    cmake -Dmaid_build_tests .

    make && make test

    # example

    g++ client.cpp *.pb.cc -luv -lprotobuf -lmaid -o client

    g++ server.cpp *.pb.cc -luv -lprotobuf -lmaid -o server

    ./server

    ./client
