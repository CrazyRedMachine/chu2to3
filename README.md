# chu2to3

chu2to3 is a chuniio wrapper which allows you to use chuniio.dll in chusan.

It's been tested on the RedBoard and Yubideck so far but should work with other chuniio implementations.

# Acknowledgments

Special thanks to akiroz for telling me about CreateFileMapping, and One3 for the way to the easy compilation :)

# How to use

1. Place both files in your chusan folder, **as well as your desired chuniio.dll**

2. Set the following in your tools.ini

```
[chuniio]
path32=chu2to3_x86.dll
path64=chu2to3_x64.dll
```

3. enjoy your favorite controller

# How to build

1. Install MSVC (build tools), meson & ninja
2. run `build32.bat` and `build64.bat` 
3. retrieve your files in `bin` folder

Note: one might have to update the path for vcvarsall.bat in both build32.bat and build64.bat, 
I'm using MSVC 2022 so it currently is `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\` for me
