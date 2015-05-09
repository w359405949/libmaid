protoc --cpp_out=. *.proto --proto_path=$HOME/.local/include/ --proto_path=.

g++ client.cpp *.cc -o client -lmaid -luv -lprotobuf -ggdb -O0 -I../../include
g++ server.cpp *.cc -o server -lmaid -luv -lprotobuf -ggdb -O0 -I../../include
