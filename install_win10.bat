@echo off

SET xbmcdir="c:\xbmc\xbmc"

if exist %appdata%\Kodi\addons\pvr.hdhomerun rmdir /s /q %appdata%\Kodi\addons\pvr.hdhomerun
mkdir %appdata%\Kodi\addons\pvr.hdhomerun
xcopy %xbmcdir%\addons\pvr.hdhomerun %appdata%\Kodi\addons\pvr.hdhomerun /s /e /h 
rmdir /s /q %xbmcdir%\addons\pvr.hdhomerun