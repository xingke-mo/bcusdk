/*
    BCU SDK bcu development enviroment
    Copyright (C) 2005 Martin K�gler <mkoegler@auto.tuwien.ac.at>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MAP_H
#define MAP_H

#include "common.h"

itype Map_ProfileID (const String & s);
ftype Map_DPTType (const String & s);
GroupType Map_GroupType (const String & s);
PropertyType Map_PropertyType (const String & s);
BCUType Map_BCUType (const String & s);
itype Map_PropertyID (const String & s);
itype Map_ObjectType (const String & s);

String escapeString (const String & s);
const char *unMap_GroupType (GroupType s);
const char *unMap_PropertyType (PropertyType s);
const char *unMap_BCUType (BCUType s);

#endif