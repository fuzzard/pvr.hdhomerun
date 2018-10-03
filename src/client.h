#pragma once
/*
 *      Copyright (C) 2015 Zoltan Csizmadia <zcsizmadia@gmail.com>
 *      https://github.com/zcsizmadia/pvr.hdhomerun
 *
 *      Copyright (C) 2011 Pulse-Eight
 *      http://www.pulse-eight.com/
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <libXBMC_addon.h>
#include <libXBMC_pvr.h>

class HDHomeRunTuners;

struct SettingsType
{
  SettingsType()
  {
    bHideProtected = true;
    bHideDuplicateChannels = true;
    bDebug = false;
    bMarkNew = false;
    bRecording = true;
    strRecPath = "special://recordings/";
  }

  bool bHideProtected;
  bool bHideDuplicateChannels;
  bool bDebug;
  bool bMarkNew;
  bool bRecording;
  std::string strRecPath;
};

struct GlobalsType
{
  GlobalsType()
  {
    currentStatus = ADDON_STATUS_UNKNOWN;
    XBMC = NULL;
    PVR = NULL;
    Tuners = NULL;
  }

  ADDON_STATUS currentStatus;
  ADDON::CHelper_libXBMC_addon* XBMC;
  CHelper_libXBMC_pvr* PVR;

  HDHomeRunTuners* Tuners;

  SettingsType Settings;
};

extern GlobalsType g;
