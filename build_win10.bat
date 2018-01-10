@echo off

SET xbmcdir="c:\xbmc\xbmc"

cd c:\xbmc\pvr.hdhomerun
rmdir /s /q build
mkdir build
cd build
cmake -G "Visual Studio 14 2015 Win64" -DADDONS_TO_BUILD=pvr.hdhomerun -DADDON_SRC_PREFIX=..\.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=..\..\xbmc\addons -DCMAKE_USER_MAKE_RULES_OVERRIDE=%xbmcdir%\cmake\scripts\windows\CFlagOverrides.cmake -DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX=%xbmcdir%\cmake\scripts\windows\CXXFlagOverrides.cmake -DPACKAGE_ZIP=1 ..\..\xbmc\cmake\addons
cmake --build . --config Debug
cd c:\xbmc\pvr.hdhomerun
