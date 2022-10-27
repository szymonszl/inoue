# Building on Windows

## Warning: this document is intended for developers. If you are just a user, please check the [Releases](https://github.com/szymonszl/inoue/releases) to get the program.

### Warning: This is out of date. I have been working on a better process, but it's not yet ready enough to document. (Spoiler: MSYS)

I am not normally a Windows developer, and it took me a lot of trouble to figure out how to build Inoue on Windows. If you know how to simplify this process, please get in touch!

You will need Visual Studio with CMake and vcpkg.

Install curl with vcpkg: `vcpkg install curl:x64-windows`. Make sure you have integrated vcpkg with your copy of VS.

Then, clone the inoue repository, and create a file called `CMakeLists.txt` with the following contents:

```
cmake_minimum_required(VERSION 3.7)
set(CMAKE_TOOLCHAIN_FILE YOUR_PATH_TO_VCPKG/buildsystems/vcpkg.cmake)
project(inoue)

file(GLOB_RECURSE inoue_src
    "src/*.c"
    "src/*.h"
)

find_package(CURL CONFIG REQUIRED)

add_executable(inoue ${inoue_src})
target_include_directories(inoue
    PRIVATE ${PROJECT_SOURCE_DIR}/src
    PRIVATE ${CURL_INCLUDE_DIR})
target_link_libraries(inoue ${CURL_LIBRARIES})
install(TARGETS inoue RUNTIME DESTINATION bin)

target_link_libraries(inoue wsock32 ws2_32)
```

Remember to change `YOUR_PATH_TO_VCPKG`!

You should then be able to compile Inoue, and find the EXE and two DLLs in the build output folder. You can use them as normal!

The release I post contains the following files:

```
inoue/
|- bin/
|  |- inoue.exe
|  |- libcurl.dll
|  |- zlib1.dll
|- inoue.bat
|- inoue.cfg
|- README.md
```

`inoue.cfg` is just `inoue.cfg.example` renamed to make editing easier. `inoue.bat` is a the following launcher:

```
@title Inoue
@"%~dp0/bin/inoue.exe" %~dp0
@pause
```

It is included only to make sure the output directory is correct on Windows, and so the program doesn't exit straight after failure.
