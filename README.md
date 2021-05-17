# Osspan
用于同步本地文件夹和远程阿里云aliyun OSS目录之间文件的GUI工具。虽然一开始就设计为可以跨平台运行，但是目前只支持macOS。

## Screeshot
<img width="1200" alt="Osspan-screenshot" src="https://user-images.githubusercontent.com/1928025/118569960-aaefce80-b7ad-11eb-8042-a85830d6718b.png">

## build

1. `git clone --recursive https://github.com/dolmens/Osspan.git`
2. `brew install openssl wxmac`
3. `cd Osspan`

4. 临时修改osssdk/CMakeLists.txt，将
```
 list(APPEND SDK_COMPILER_FLAGS "-fno-exceptions" "-fPIC" "-fno-rtti")
```
修改为
```
 list(APPEND SDK_COMPILER_FLAGS "-fno-exceptions" "-fPIC")
```

5. make
```
cmake -B build -DCMAKE_BUILD_TYPE=Release \
   -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl \
   -DOPENSSL_LIBRARIES=/usr/local/opt/openssl/lib \
   -DOPENSSL_INCLUDE_DIRS=/usr/local/opt/openssl/include

 cmake --build build
```
5. run
```
./build/src/Osspan.app/Contents/MacOS/Osspan
```
