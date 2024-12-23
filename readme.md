HDR Demo

# 编译说明
1. 安装Visual Studio 2019/2022、CMake
2. 安装vcpkg 并同时安装 OpenCV
3. 安装海康MVS软件
4. 配置项目 `cmake . -DCMAKE_TOOLCHAIN_FILE=D:/vcpkg/scripts/buildsystems/vcpkg.cmake`，注意这里的vcpkg路径替换为你实际的路径
5. 编译项目 `cmake --build . --config Release` 或者在Visual Studio中编译