protoc --cpp_out=. *.proto --proto_path=$HOME/.local/include/ --proto_path=.

g++ client.cpp *.cc -o client -lmaid -luv -lprotobuf
g++ server.cpp *.cc -o server -lmaid -luv -lprotobuf
