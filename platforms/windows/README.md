Windows (command line)
======================
First installing Git, Visual Studio 2015 Update 3 or newer
To build a runnable Windows application bundle, run:

```cmd
mkdir build\windows
cd build\windows
cmake -G "Visual Studio 14 2015 Win64" ..\.. -DPLATFORM_TARGET=windows
```
And open the application under build/windows/bin/tangram.exe

When debugging in the Visual Studio
Make sure the Working Directory to be `$(TargetDir)` instead of `${ProjectDir}`
