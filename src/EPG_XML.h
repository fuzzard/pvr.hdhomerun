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

#ifndef EPG_XML_H
#define EPG_XML_H

#include "EPG.h"
#include "rapidxml/rapidxml.hpp"

#include <vector>

using namespace rapidxml;

class EPG_XML : public EPGBase
{

  public:
    EPG_XML(){};
    virtual ~EPG_XML(){};
    virtual bool UpdateGuide(HDHomeRunTuners::Tuner*, String);

  protected:
    bool GzipInflate(const String &compressedBytes, String &uncompressedBytes);
    bool _xmlparse(HDHomeRunTuners::Tuner *pTuner, char *xmlbuffer);
    Json::Value& findJsonValue(Json::Value &Guide, String jsonElement, String searchData);
    int ParseDateTime(String& strDate);

  private:
    void _prepareGuide(HDHomeRunTuners::Tuner *pTuner);
    void _duplicateChannelCheck(HDHomeRunTuners::Tuner *pTuner);
    bool _xmlparseelement(HDHomeRunTuners::Tuner *pTuner,const xml_node<> *pRootNode, const char *strElement);
    bool _useSDicons(HDHomeRunTuners::Tuner *pTuner);
    std::map<String, std::vector<Json::Value*>> channelMap;
    int tempSeriesId = 0;
    Json::Value jsonNull = Json::Value::null;
};

#endif
