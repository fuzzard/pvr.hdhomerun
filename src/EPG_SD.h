/*
 *      Copyright (C) 2017 Brent Murphy <bmurphy@bcmcs.net>
 *      https://github.com/fuzzard/pvr.hdhomerun
 *
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
#ifndef EPG_SD_H
#define EPG_SD_H

#include "EPG.h"

class CEpg_SD : public CEpgBase
{

  public:
    CEpg_SD(){};
    virtual ~CEpg_SD(){};
    virtual bool UpdateGuide(HDHomeRunTuners::Tuner*, String);

  private:
    unsigned long long GetEndTime(Json::Value);
    bool InsertGuideData(Json::Value&, Json::Value);
    bool UpdateAdvancedGuide(HDHomeRunTuners::Tuner*, String);
    bool UpdateBasicGuide(HDHomeRunTuners::Tuner*, String);

};

#endif