

# 1. Use the tools from the Standalone Toolchain
export PATH=$NDKROOT/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64/bin:$PATH
export SYSROOT=$NDKROOT/platforms/android-21/arch-arm64
export CC="arm-linux-androideabi-gcc --sysroot $SYSROOT"
export CXX="arm-linux-androideabi-g++ --sysroot $SYSROOT"
export CXXSTL=$NDKROOT/platforms/android-21/arch-arm64/


cd deps/protobuf
mkdir -p android
./configure --prefix=${pwd}/android \
    --host=arm-linux-androideabi \
    --with-sysroot=$SYSROOT \
    --disable-shared \
    --enable-cross-compile \
    --with-protoc=protoc \
    CFLAGS="-march=armv7-a" \
    CXXFLAGS="-march=armv7-a -I$CXXSTL/include -I$CXXSTL/libs/armeabi-v7a/include"

# 4. Build
make && make install

# 5. Inspect the library architecture specific information
arm-linux-androideabi-readelf -A android/libprotobuf-lite.a
