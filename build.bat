mkdir build
cd build
cmake .. -G "Visual Studio 15 2017" -A Win32
MSBuild.exe Maestro.sln /p:Configuration=Release /p:Platform=Win32 /clp:ErrorsOnly
cd Release
TestEntry.exe