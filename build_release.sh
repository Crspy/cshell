type="Release"
name="cshell"
mkdir -p ./build
mkdir -p ./build/$type
mkdir -p ./bin
mkdir -p ./bin/$type
cd ./build/$type
cmake -DCMAKE_BUILD_TYPE=$type ../../
make
mv ./$name ../../bin/$type/$name