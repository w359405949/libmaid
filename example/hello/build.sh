protoc --cpp_out=. *.proto --proto_path=/home/wcq/.local/include/ --proto_path=.

g++ client.cpp *.cc -o client -lmaid
g++ server.cpp *.cc -o server -lmaid
