/*
    EIBD eib bus access and management daemon
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

#include "managementclient.h"
#include "management.h"

void
ReadIndividualAddresses (Layer3 * l3, Trace * t, ClientConnection * c,
			 pth_event_t stop)
{
  try
  {
    Layer7_Broadcast b (l3, t);
    CArray erg;
    Array < eibaddr_t > e = b.A_IndividualAddress_Read ();
    erg.resize (2 + 2 * e ());
    EIBSETTYPE (erg, EIB_M_INDIVIDUAL_ADDRESS_READ);
    for (unsigned i = 0; i < e (); i++)
      {
	erg[2 + i * 2] = (e[i] >> 8) & 0xff;
	erg[2 + i * 2 + 1] = (e[i]) & 0xff;
      }
    c->sendmessage (erg (), erg.array (), stop);
  }
  catch (Exception e)
  {
    c->sendreject (stop, EIB_PROCESSING_ERROR);
  }
}

void
ChangeProgMode (Layer3 * l3, Trace * t, ClientConnection * c,
		pth_event_t stop)
{
  try
  {
    eibaddr_t dest;
    uchar res[3];
    int i;
    EIBSETTYPE (res, EIB_PROG_MODE);
    res[2] = 0;
    if (c->size < 5)
      {
	c->sendreject (stop);
	return;
      }
    dest = (c->buf[2] << 8) | (c->buf[3]);
    Management_Connection m (l3, t, dest);
    switch (c->buf[4])
      {
      case 0:
	if (m.X_Progmode_Off () == -1)
	  c->sendreject (stop);
	else
	  c->sendmessage (3, res, stop);
	break;
      case 1:
	if (m.X_Progmode_On () == -1)
	  c->sendreject (stop);
	else
	  c->sendmessage (3, res, stop);
	break;
      case 2:
	if (m.X_Progmode_Toggle () == -1)
	  c->sendreject (stop);
	else
	  c->sendmessage (3, res, stop);
	break;
      case 3:
	if ((i = m.X_Progmode_Status ()) == -1)
	  c->sendreject (stop);
	else
	  {
	    res[2] = i;
	    c->sendmessage (3, res, stop);
	  }
	break;
      default:
	c->sendreject (stop);
      }
  }
  catch (Exception e)
  {
    c->sendreject (stop, EIB_PROCESSING_ERROR);
  }
}

void
GetMaskVersion (Layer3 * l3, Trace * t, ClientConnection * c,
		pth_event_t stop)
{
  try
  {
    eibaddr_t dest;
    uchar res[4];
    uint16_t maskver;
    EIBSETTYPE (res, EIB_MASK_VERSION);
    res[2] = 0;
    if (c->size < 4)
      {
	c->sendreject (stop);
	return;
      }

    dest = (c->buf[2] << 8) | (c->buf[3]);
    Management_Connection m (l3, t, dest);
    if (m.A_Device_Descriptor_Read (maskver) == -1)
      c->sendreject (stop);
    else
      {
	res[2] = (maskver >> 8) & 0xff;
	res[3] = (maskver) & 0xff;
	c->sendmessage (4, res, stop);
      }
  }
  catch (Exception e)
  {
    c->sendreject (stop, EIB_PROCESSING_ERROR);
  }
}

void
WriteIndividualAddress (Layer3 * l3, Trace * t, ClientConnection * c,
			pth_event_t stop)
{
  try
  {
    eibaddr_t dest;
    uint16_t maskver;
    if (c->size < 4)
      {
	c->sendreject (stop);
	return;
      }

    dest = (c->buf[2] << 8) | (c->buf[3]);
    Layer7_Broadcast b (l3, t);
    {
      Management_Connection m (l3, t, dest);
      if (m.A_Device_Descriptor_Read (maskver) != -1)
	{
	  c->sendreject (stop, EIB_ERROR_ADDR_EXISTS);
	  return;
	}
    }
    Array < eibaddr_t > addr = b.A_IndividualAddress_Read ();
    if (addr () > 1)
      {
	c->sendreject (stop, EIB_ERROR_MORE_DEVICE);
	return;
      }
    if (addr () == 0)
      {
	c->sendreject (stop, EIB_ERROR_TIMEOUT);
	return;
      }
    b.A_IndividualAddress_Write (dest);

    Management_Connection m1 (l3, t, dest);
    if (m1.A_Device_Descriptor_Read (maskver) == -1)
      {
	c->sendreject (stop, EIB_PROCESSING_ERROR);
	return;
      }
    if (m1.X_Progmode_Off () == -1)
      {
	c->sendreject (stop, EIB_PROCESSING_ERROR);
	return;
      }
    c->sendreject (stop, EIB_M_INDIVIDUAL_ADDRESS_WRITE);
  }
  catch (Exception e)
  {
    c->sendreject (stop, EIB_PROCESSING_ERROR);
  }
}
void
ManagementConnection (Layer3 * l3, Trace * t, ClientConnection * c,
		      pth_event_t stop)
{
  try
  {
    eibaddr_t dest;
    uint16_t maskver;
    int16_t val;
    uchar buf[10];
    int i;
    if (c->size < 4)
      {
	c->sendreject (stop);
	return;
      }

    dest = (c->buf[2] << 8) | (c->buf[3]);
    Management_Connection m (l3, t, dest);
    if (m.A_Device_Descriptor_Read (maskver) == -1)
      {
	c->sendreject (stop);
	return;
      }
    c->sendreject (stop, EIB_MC_CONNECTION);
    do
      {
	i = c->readmessage (stop);
	if (i != -1)
	  switch (EIBTYPE (c->buf))
	    {
	    case EIB_MC_PROG_MODE:
	      if (c->size < 3)
		{
		  c->sendreject (stop);
		  break;
		}
	      EIBSETTYPE (buf, EIB_MC_PROG_MODE);
	      buf[2] = 0;
	      switch (c->buf[2])
		{
		case 0:
		  if (m.X_Progmode_Off () == -1)
		    c->sendreject (stop);
		  else
		    c->sendmessage (3, buf, stop);
		  break;
		case 1:
		  if (m.X_Progmode_On () == -1)
		    c->sendreject (stop);
		  else
		    c->sendmessage (3, buf, stop);
		  break;
		case 2:
		  if (m.X_Progmode_Toggle () == -1)
		    c->sendreject (stop);
		  else
		    c->sendmessage (3, buf, stop);
		  break;
		case 3:
		  if ((i = m.X_Progmode_Status ()) == -1)
		    c->sendreject (stop);
		  else
		    {
		      buf[2] = i;
		      c->sendmessage (3, buf, stop);
		    }
		  break;
		default:
		  c->sendreject (stop);
		}
	      break;
	    case EIB_MC_MASK_VERSION:
	      if (m.A_Device_Descriptor_Read (maskver) == -1)
		c->sendreject (stop);
	      else
		{
		  EIBSETTYPE (buf, EIB_MC_MASK_VERSION);
		  buf[2] = (maskver >> 8) & 0xff;
		  buf[3] = (maskver) & 0xff;
		  c->sendmessage (4, buf, stop);
		}
	      break;
	    case EIB_MC_PEI_TYPE:
	      if (m.X_Get_PEIType (val) == -1)
		c->sendreject (stop);
	      else
		{
		  EIBSETTYPE (buf, EIB_MC_PEI_TYPE);
		  buf[2] = (val >> 8) & 0xff;
		  buf[3] = (val) & 0xff;
		  c->sendmessage (4, buf, stop);
		}
	      break;
	    case EIB_MC_ADC_READ:
	      if (c->size < 4)
		{
		  c->sendreject (stop);
		  break;
		}
	      if (m.A_ADC_Read (c->buf[2], c->buf[3], val) == -1)
		c->sendreject (stop);
	      else
		{
		  EIBSETTYPE (buf, EIB_MC_ADC_READ);
		  buf[2] = (val >> 8) & 0xff;
		  buf[3] = (val) & 0xff;
		  c->sendmessage (4, buf, stop);
		}
	      break;

	    case EIB_MC_READ:
	      if (c->size < 6)
		{
		  c->sendreject (stop);
		  break;
		}
	      {
		memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
		unsigned len = (c->buf[4] << 8) | (c->buf[5]);
		CArray data, erg;
		if (m.X_Memory_Read_Block (addr, len, data) == -1)
		  c->sendreject (stop);
		else
		  {
		    erg.resize (6);
		    EIBSETTYPE (erg, EIB_MC_READ);
		    erg.setpart (data, 2);
		    c->sendmessage (erg (), erg.array (), stop);
		  }
	      }
	      break;

	    case EIB_MC_WRITE:
	      if (c->size < 6)
		{
		  c->sendreject (stop);
		  break;
		}
	      {
		memaddr_t addr = (c->buf[2] << 8) | (c->buf[3]);
		unsigned len = (c->buf[4] << 8) | (c->buf[5]);
		if (c->size < len + 6)
		  {
		    c->sendreject (stop);
		    break;
		  }
		if (m.X_Memory_Write_Block (addr, CArray (c->buf + 6, len)) ==
		    -1)
		  c->sendreject (stop);
		else
		  c->sendreject (stop, EIB_MC_WRITE);
	      }
	      break;

	    case EIB_MC_PROP_READ:
	      if (c->size < 7)
		{
		  c->sendreject (stop);
		  break;
		}
	      {
		CArray data, erg;
		if (m.
		    A_Property_Read (c->buf[2], c->buf[3],
				     (c->buf[4] << 8) | c->buf[5], c->buf[6],
				     data) == -1)
		  c->sendreject (stop);
		else
		  {
		    erg.resize (2);
		    EIBSETTYPE (erg, EIB_MC_PROP_READ);
		    erg.setpart (data, 2);
		    c->sendmessage (erg (), erg.array (), stop);
		  }
	      }
	      break;

	    case EIB_MC_PROP_WRITE:
	      if (c->size < 7)
		{
		  c->sendreject (stop);
		  break;
		}
	      if (m.
		  X_Property_Write (c->buf[2], c->buf[3],
				    (c->buf[4] << 8) | c->buf[5], c->buf[6],
				    CArray (c->buf + 7, c->size - 7)) == -1)
		c->sendreject (stop);
	      else
		c->sendreject (stop, EIB_MC_PROP_WRITE);
	      break;

	    case EIB_MC_AUTHORIZE:
	      if (c->size < 6)
		{
		  c->sendreject (stop);
		  break;
		}
	      EIBSETTYPE (buf, EIB_MC_AUTHORIZE);
	      if (m.A_Authorize (c->buf + 2, buf[3]) == -1)
		c->sendreject (stop);
	      else
		c->sendmessage (3, buf, stop);
	      break;

	    case EIB_MC_KEY_WRITE:
	      if (c->size < 7)
		{
		  c->sendreject (stop);
		  break;
		}
	      if (m.A_KeyWrite (c->buf + 2, *(c->buf + 6)) == -1)
		c->sendreject (stop);
	      else
		c->sendreject (stop, EIB_MC_KEY_WRITE);
	      break;

	    case EIB_MC_PROP_DESC:
	      if (c->size < 4)
		{
		  c->sendreject (stop);
		  break;
		}
	      if (m.
		  A_Property_Desc (c->buf[2], c->buf[3], 0, buf[2], maskver,
				   buf[5]) == -1)
		c->sendreject (stop);
	      else
		{
		  EIBSETTYPE (buf, EIB_MC_PROP_DESC);
		  buf[3] = (maskver >> 8) & 0xff;
		  buf[4] = (maskver) & 0xff;
		  c->sendmessage (6, buf, stop);
		}
	      break;

	    case EIB_MC_PROP_SCAN:
	      {
		Array < PropertyInfo > p;
		if (m.X_PropertyScan (p) == -1)
		  c->sendreject (stop);
		else
		  {
		    CArray erg;
		    erg.resize (2 + p () * 6);
		    EIBSETTYPE (erg, EIB_MC_PROP_SCAN);
		    for (unsigned i = 0; i < p (); i++)
		      {
			erg[i * 6 + 2] = p[i].obj;
			erg[i * 6 + 3] = p[i].property;
			erg[i * 6 + 4] = p[i].type;
			erg[i * 6 + 5] = (p[i].count >> 8) & 0xff;
			erg[i * 6 + 6] = (p[i].count) & 0xff;
			erg[i * 6 + 7] = p[i].access;
		      }
		    c->sendmessage (erg (), erg.array (), stop);
		  }
	      }
	      break;

	    default:
	      c->sendreject (stop);
	    }
      }
    while (i != -1);
  }
  catch (Exception e)
  {
    c->sendreject (stop);
  }
}