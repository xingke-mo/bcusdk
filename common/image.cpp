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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "loadctl.h"
#include "image.h"

String
HexDump (CArray data)
{
  char buf[200];
  String s;
  int i;
  sprintf (buf, "%04X ", 0);
  s = buf;
  for (i = 0; i < data (); i++)
    {
      sprintf (buf, "%02x ", data[i]);
      s += buf;
      if (i % 16 == 15)
	{
	  sprintf (buf, "\n%04X ", i + 1);
	  s += buf;
	}
    }
  s += "\n";
  return s;
}

STR_Stream *
STR_Stream::fromArray (const CArray & c)
{
  assert (c () >= 4);
  assert (((c[0] << 8) | c[1]) + 2 == c ());
  try
  {
    switch (c[2] << 8 | c[3])
      {
      case L_STRING_PAR:
	return new STR_StringParameter (c);
      case L_INT_PAR:
	return new STR_IntParameter (c);
      case L_FLOAT_PAR:
	return new STR_FloatParameter (c);
      case L_LIST_PAR:
	return new STR_ListParameter (c);
      case L_GROUP_OBJECT:
	return new STR_GroupObject (c);
      case L_BCU_TYPE:
	return new STR_BCUType (c);
      case L_BCU2_INIT:
	return new STR_BCU2Start (c);
      case L_CODE:
	return new STR_Code (c);
      case L_BCU1_SIZE:
	return new STR_BCU1Size (c);
      default:
	return new STR_Unknown (c);
      }
  }
  catch (...)
  {
    return new STR_Invalid (c);
  }
}

STR_Invalid::STR_Invalid ()
{
}

STR_Invalid::STR_Invalid (const CArray & c)
{
  data = c;
}

CArray
STR_Invalid::toArray ()
{
  return data;
}

String
STR_Invalid::decode ()
{
  char buf[200];
  String s;
  sprintf (buf, "Invalid:\n");
  s = buf;
  return s + HexDump (data);
}


STR_Unknown::STR_Unknown ()
{
  type = 0;
}

STR_Unknown::STR_Unknown (const CArray & c)
{
  data.set (c.array () + 4, c () - 4);
  type = c[2] << 8 | c[3];
}

CArray
STR_Unknown::toArray ()
{
  CArray d;
  uint16_t len = 2 + data ();
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (type >> 8) & 0xff;
  d[3] = (type) & 0xff;
  d.setpart (data, 4);
  return d;
}

String
STR_Unknown::decode ()
{
  char buf[200];
  String s;
  sprintf (buf, "Unknown %04x:\n", type);
  s = buf;
  return s + HexDump (data);
}

STR_BCUType::STR_BCUType ()
{
  bcutype = 0;
}

STR_BCUType::STR_BCUType (const CArray & c)
{
  if (c () != 6)
    throw 1;
  bcutype = c[4] << 8 | c[5];
}

CArray
STR_BCUType::toArray ()
{
  CArray d;
  uint16_t len = 4;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU_TYPE >> 8) & 0xff;
  d[3] = (L_BCU_TYPE) & 0xff;
  d[4] = (bcutype >> 8) & 0xff;
  d[5] = (bcutype) & 0xff;
  return d;
}

String
STR_BCUType::decode ()
{
  char buf[200];
  sprintf (buf, "Maskversion: %04x\n", bcutype);
  return buf;
}

STR_Code::STR_Code ()
{
}

STR_Code::STR_Code (const CArray & c)
{
  code.set (c.array () + 4, c () - 4);
}

CArray
STR_Code::toArray ()
{
  CArray d;
  uint16_t len = 2 + code ();
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_CODE >> 8) & 0xff;
  d[3] = (L_CODE) & 0xff;
  d.setpart (code, 4);
  return d;
}

String
STR_Code::decode ()
{
  char buf[200];
  String s;
  int i;
  sprintf (buf, "Code:\n");
  s = buf;
  return s + HexDump (code);
}

STR_StringParameter::STR_StringParameter ()
{
  addr = 0;
  length = 0;
}

STR_StringParameter::STR_StringParameter (const CArray & c)
{
  const uchar *d;
  if (c () < 9)
    throw 1;
  addr = c[4] << 8 | c[5];
  length = c[6] << 8 | c[7];
  if (c[c () - 1])
    throw 1;
  d = &c[8];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    throw 1;
  name = (const char *) c.array () + 8;
}

CArray
STR_StringParameter::toArray ()
{
  CArray d;
  uint16_t len = 7 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_STRING_PAR >> 8) & 0xff;
  d[3] = (L_STRING_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d[6] = (length >> 8) & 0xff;
  d[7] = (length) & 0xff;
  d.setpart ((const uchar *) name (), 8, strlen (name ()) + 1);
  return d;
}

String
STR_StringParameter::decode ()
{
  char buf[200];
  sprintf (buf, "StringParameter: addr=%04x id=%s length=%d\n", addr, name (),
	   length);
  return buf;
}

STR_IntParameter::STR_IntParameter ()
{
  name = 0;
  type = 0;
}

STR_IntParameter::STR_IntParameter (const CArray & c)
{
  const uchar *d;
  if (c () < 8)
    throw 1;
  addr = c[4] << 8 | c[5];
  type = (int8_t) c[6];
  if (c[c () - 1])
    throw 1;
  d = &c[7];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    throw 1;
  name = (const char *) c.array () + 7;
}

CArray
STR_IntParameter::toArray ()
{
  CArray d;
  uint16_t len = 6 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_INT_PAR >> 8) & 0xff;
  d[3] = (L_INT_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d[6] = (type) & 0xff;
  d.setpart ((const uchar *) name (), 7, strlen (name ()) + 1);
  return d;
}

String
STR_IntParameter::decode ()
{
  char buf[200];
  sprintf (buf, "IntParameter: addr=%04x id=%s type=%s %d bytes\n", addr,
	   name (), type < 0 ? "signed" : "unsigned", 1 << (abs (type) - 1));
  return buf;
}

STR_FloatParameter::STR_FloatParameter ()
{
  addr = 0;
}

STR_FloatParameter::STR_FloatParameter (const CArray & c)
{
  const uchar *d;
  if (c () < 7)
    throw 1;
  addr = c[4] << 8 | c[5];
  if (c[c () - 1])
    throw 1;
  d = &c[6];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    throw 1;
  name = (const char *) c.array () + 6;
}

CArray
STR_FloatParameter::toArray ()
{
  CArray d;
  uint16_t len = 5 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_FLOAT_PAR >> 8) & 0xff;
  d[3] = (L_FLOAT_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d.setpart ((const uchar *) name (), 6, strlen (name ()) + 1);
  return d;
}

String
STR_FloatParameter::decode ()
{
  char buf[200];
  sprintf (buf, "FloatParameter: addr=%04x id=%s\n", addr, name ());
  return buf;
}

STR_ListParameter::STR_ListParameter ()
{
  addr = 0;
}

STR_ListParameter::STR_ListParameter (const CArray & c)
{
  uint16_t el, i;
  const uchar *d, *d1;
  if (c () < 9)
    throw 1;
  addr = c[4] << 8 | c[5];
  el = c[6] << 8 | c[7];
  if (c[c () - 1])
    throw 1;
  d = &c[8];
  d1 = d;
  while (*d)
    d++;
  if (d > &c[c () - 1])
    throw 1;
  name = (const char *) d1;
  d1 = ++d;
  if (d > &c[c () - 1])
    throw 1;
  elements.resize (el);
  for (i = 0; i < el; i++)
    {
      while (*d)
	d++;
      if (d > &c[c () - 1])
	throw 1;
      elements[i] = (const char *) d1;
      d1 = ++d;
      if (d > &c[c ()])
	throw 1;
    }
  if (d != &c[c ()])
    throw 1;
}

CArray
STR_ListParameter::toArray ()
{
  CArray d;
  uint16_t i, p;
  uint16_t len = 7 + strlen (name ());
  for (i = 0; i < elements (); i++)
    len += strlen (elements[i] ()) + 1;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_LIST_PAR >> 8) & 0xff;
  d[3] = (L_LIST_PAR) & 0xff;
  d[4] = (addr >> 8) & 0xff;
  d[5] = (addr) & 0xff;
  d[6] = (elements () >> 8) & 0xff;
  d[7] = (elements ()) & 0xff;
  d.setpart ((const uchar *) name (), 8, strlen (name ()) + 1);
  p = 8 + strlen (name ()) + 1;
  for (i = 0; i < elements (); p += strlen (elements[i] ()) + 1, i++)
    d.setpart ((const uchar *) elements[i] (), p,
	       strlen (elements[i] ()) + 1);
  return d;
}

String
STR_ListParameter::decode ()
{
  char buf[200];
  String s;
  sprintf (buf, "ListParameter: addr=%04x id=%s elements=", addr, name ());
  s = buf;
  for (int i = 0; i < elements (); i++)
    {
      sprintf (buf, "%s,", elements[i] ());
      s += buf;
    }
  s += "\n";
  return s;
}

STR_GroupObject::STR_GroupObject ()
{
  no = 0;
}

STR_GroupObject::STR_GroupObject (const CArray & c)
{
  const uchar *d;
  if (c () < 6)
    throw 1;
  no = c[4];
  if (c[c () - 1])
    throw 1;
  d = &c[5];
  while (*d)
    d++;
  if (d != &c[c () - 1])
    throw 1;
  name = (const char *) c.array () + 5;
}

String
STR_GroupObject::decode ()
{
  char buf[200];
  sprintf (buf, "GROUP_OBJECT %d: id=%s\n", no, name ());
  return buf;
}

CArray
STR_GroupObject::toArray ()
{
  CArray d;
  uint16_t len = 4 + strlen (name ());
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_GROUP_OBJECT >> 8) & 0xff;
  d[3] = (L_GROUP_OBJECT) & 0xff;
  d[4] = (no) & 0xff;
  d.setpart ((const uchar *) name (), 5, strlen (name ()) + 1);
  return d;
}

STR_BCU1Size::STR_BCU1Size ()
{
  textsize = 0;
  stacksize = 0;
  datasize = 0;
  bsssize = 0;
}

STR_BCU1Size::STR_BCU1Size (const CArray & c)
{
  if (c () != 12)
    throw 1;
  textsize = c[4] << 8 | c[5];
  stacksize = c[6] << 8 | c[7];
  datasize = c[8] << 8 | c[9];
  bsssize = c[10] << 8 | c[11];
}

CArray
STR_BCU1Size::toArray ()
{
  CArray d;
  uint16_t len = 10;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU1_SIZE >> 8) & 0xff;
  d[3] = (L_BCU1_SIZE) & 0xff;
  d[4] = (textsize >> 8) & 0xff;
  d[5] = (textsize) & 0xff;
  d[6] = (stacksize >> 8) & 0xff;
  d[7] = (stacksize) & 0xff;
  d[8] = (datasize >> 8) & 0xff;
  d[9] = (datasize) & 0xff;
  d[10] = (bsssize >> 8) & 0xff;
  d[11] = (bsssize) & 0xff;
  return d;
}

String
STR_BCU1Size::decode ()
{
  char buf[200];
  sprintf (buf, "BCU1_SIZE: text:%d stack:%d data:%d bss:%d\n", textsize,
	   stacksize, datasize, bsssize);
  return buf;
}

STR_BCU2Start::STR_BCU2Start ()
{
  initaddr = 0;
  runaddr = 0;
  saveaddr = 0;
}

STR_BCU2Start::STR_BCU2Start (const CArray & c)
{
  if (c () != 10)
    throw 1;
  initaddr = c[4] << 8 | c[5];
  runaddr = c[6] << 8 | c[7];
  saveaddr = c[8] << 8 | c[9];
}

CArray
STR_BCU2Start::toArray ()
{
  CArray d;
  uint16_t len = 8;
  d.resize (2 + len);
  d[0] = (len >> 8) & 0xff;
  d[1] = (len) & 0xff;
  d[2] = (L_BCU2_INIT >> 8) & 0xff;
  d[3] = (L_BCU2_INIT) & 0xff;
  d[4] = (initaddr >> 8) & 0xff;
  d[5] = (initaddr) & 0xff;
  d[6] = (runaddr >> 8) & 0xff;
  d[7] = (runaddr) & 0xff;
  d[8] = (saveaddr >> 8) & 0xff;
  d[9] = (saveaddr) & 0xff;
  return d;
}

String
STR_BCU2Start::decode ()
{
  char buf[200];
  sprintf (buf, "BCU2_INIT: init:%04X run:%04X save:%04X\n", initaddr,
	   runaddr, saveaddr);
  return buf;
}

Image::Image ()
{
}

Image::~Image ()
{
  for (int i = 0; i < str (); i++)
    if (str[i])
      delete str[i];
}

String
Image::decode ()
{
  String s = "BCU Memory Image\n";
  for (int i = 0; i < str (); i++)
    s += str[i]->decode ();
  return s;
}

CArray
Image::toArray ()
{
  CArray data;
  data.resize (10);
  data[0] = 0xbc;
  data[1] = 0x68;
  data[2] = 0x0c;
  data[3] = 0x05;
  data[4] = 0xbc;
  data[5] = 0x68;
  data[6] = 0x0c;
  data[7] = 0x05;
  for (int i = 0; i < str (); i++)
    data.setpart (str[i]->toArray (), data ());
  data[8] = (data () >> 8) & 0xff;
  data[9] = (data ()) & 0xff;
  return data;
}

int
Image::findStreamNumber (STR_Type t)
{
  for (int i = 0; i < str (); i++)
    if (str[i]->getType () == t)
      return i;
  return -1;
}

STR_Stream *
Image::findStream (STR_Type t)
{
  int i = findStreamNumber (t);
  if (i == -1)
    return 0;
  else
    return str[i];
}

Image *
Image::fromArray (CArray c)
{
  uint16_t pos = 10;
  uint16_t len;
  if (c () < 10)
    return 0;
  if (c[0] != 0xbc)
    return 0;
  if (c[1] != 0x68)
    return 0;
  if (c[2] != 0x0c)
    return 0;
  if (c[3] != 0x05)
    return 0;
  if (c[4] != 0xbc)
    return 0;
  if (c[5] != 0x68)
    return 0;
  if (c[6] != 0x0c)
    return 0;
  if (c[7] != 0x05)
    return 0;
  if (c[8] != ((c () >> 8) & 0xff))
    return 0;
  if (c[9] != ((c ()) & 0xff))
    return 0;
  Image *i = new Image;
  while (pos < c ())
    {
      if (pos + 4 >= c ())
	{
	  delete i;
	  return 0;
	}
      len = c[pos] << 8 | c[pos + 1];
      if (pos + 2 + len > c () || len < 2)
	{
	  delete i;
	  return 0;
	}
      i->str.add (STR_Stream::fromArray (CArray (&c[pos], len + 2)));
      pos += 2 + len;
    }
  return i;
}

bool
Image::isValid ()
{
  return findStreamNumber (S_Invalid) == -1;
}