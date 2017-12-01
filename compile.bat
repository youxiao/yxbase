:: set environment
set GYP_MSVS_VERSION=2015
set DEPOT_TOOLS_WIN_TOOLCHAIN=0

:: gn gen out
call .\depot_tools\python .\depot_tools\gn.py gen .\out --ide=vs

:: gn args out
call .\depot_tools\python .\depot_tools\gn.py args .\out

:: compiling
call .\depot_tools\ninja -C .\out yxsdk

pause