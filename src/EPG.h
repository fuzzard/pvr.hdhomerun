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

//#include "EPGBase.h"
#ifndef EPG_H
#define EPG_H

#include <functional>
#include <json/json.h>
#include <map>
#include <memory>

#include "HDHomeRunTuners.h"
#include "Utils.h"


class CEpgBase
{
  public:
    virtual bool UpdateGuide(HDHomeRunTuners::Tuner*, String) = 0;

  protected:
    void AddGuideInfo(Json::Value&);
    static const String SD_GUIDEURL;
};

/*
 *  Author: John Cumming
 *  Created: 2016-02-02 Tue 21:21
 *  http://www.jsolutions.co.uk/C++/objectfactory.html
 */

// A helper class to register a factory function
class CRegistrar {
public:
    CRegistrar(String className, std::function<CEpgBase*(void)> classFactoryFunction);
};

// A preprocessor define used by derived classes
#define REGISTER_CLASS(NAME, TYPE) static CRegistrar registrar(NAME, [](void) -> CEpgBase * { return new TYPE();});

// The factory - implements singleton pattern!
class CEpgFactory
{
public:
    // Get the single instance of the factory
    static CEpgFactory * Instance();
    // register a factory function to create an instance of className
    void RegisterFactoryFunction(String name, std::function<CEpgBase*(void)> classFactoryFunction);
    // create an instance of a registered class
    std::shared_ptr<CEpgBase> Create(String name);

private:
    CEpgFactory(){}
    // the registry of factory functions
    std::map<String, std::function<CEpgBase*(void)>> factoryFunctionRegistry;

};

#endif

