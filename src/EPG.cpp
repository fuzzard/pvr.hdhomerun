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

#include "EPG.h"

/*
 *  Author: John Cumming
 *  Created: 2016-02-02 Tue 21:21
 *  http://www.jsolutions.co.uk/C++/objectfactory.html
 */

Registrar::Registrar(String name, std::function<EPGBase*(void)> classFactoryFunction)
{
    // register the class factory function
    EPGFactory::Instance()->RegisterFactoryFunction(name, classFactoryFunction);
}


EPGFactory * EPGFactory::Instance()
{
    static EPGFactory factory;
    return &factory;
}


void EPGFactory::RegisterFactoryFunction(String name, std::function<EPGBase*(void)> classFactoryFunction)
{
    // register the class factory function 
    factoryFunctionRegistry[name] = classFactoryFunction;
}


std::shared_ptr<EPGBase> EPGFactory::Create(String name)
{
  EPGBase * instance = nullptr;

  // find name in the registry and call factory method.
  auto it = factoryFunctionRegistry.find(name);
  if(it != factoryFunctionRegistry.end())
    instance = it->second();

  // wrap instance in a shared ptr and return
  if(instance != nullptr)
    return std::shared_ptr<EPGBase>(instance);
  else
    return nullptr;
}
