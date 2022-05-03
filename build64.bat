cd /d %~dp0
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
meson setup build64 --buildtype=release
ninja -C build64
mkdir bin
copy build64\src\chu2to3\chu2to3.dll bin\chu2to3_x64.dll
pause