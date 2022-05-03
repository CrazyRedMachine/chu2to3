cd /d %~dp0
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64_x86
meson setup build32 --buildtype=release
ninja -C build32
mkdir bin
copy build32\src\chu2to3\chu2to3.dll bin\chu2to3_x86.dll
pause