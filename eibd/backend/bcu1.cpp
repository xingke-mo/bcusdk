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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "bcu1.h"

BCU1DriverLowLevelDriver::BCU1DriverLowLevelDriver (const char *dev,
						    Trace * tr)
{
  t = tr;
  t->TracePrintf (1, this, "Open");
  fd = open (dev, O_RDWR);
  if (fd == -1)
    throw new Exception (DEV_OPEN_FAIL);

  pth_sem_init (&in_signal);
  pth_sem_init (&out_signal);
  pth_sem_init (&send_empty);
  pth_sem_set_value (&send_empty, 1);
  getwait = pth_event (PTH_EVENT_SEM, &out_signal);
  Start ();
  t->TracePrintf (1, this, "Opened");
}

BCU1DriverLowLevelDriver::~BCU1DriverLowLevelDriver ()
{
  t->TracePrintf (1, this, "Close");
  Stop ();
  pth_event_free (getwait, PTH_FREE_THIS);

  if (fd != -1)
    close (fd);
}

bool BCU1DriverLowLevelDriver::Connection_Lost ()
{
  return 0;
}

void
BCU1DriverLowLevelDriver::Send_Packet (CArray l)
{
  CArray pdu;
  t->TracePacket (1, this, "Send", l);
  inqueue.put (l);
  pth_sem_set_value (&send_empty, 0);
  pth_sem_inc (&in_signal, TRUE);
}

void
BCU1DriverLowLevelDriver::SendReset ()
{
}

bool BCU1DriverLowLevelDriver::Send_Queue_Empty ()
{
  return inqueue.isempty ();
}

pth_sem_t *
BCU1DriverLowLevelDriver::Send_Queue_Empty_Cond ()
{
  return &send_empty;
}

CArray *
BCU1DriverLowLevelDriver::Get_Packet (pth_event_t stop)
{
  if (stop != NULL)
    pth_event_concat (getwait, stop, NULL);

  pth_wait (getwait);

  if (stop)
    pth_event_isolate (getwait);

  if (pth_event_status (getwait) == PTH_STATUS_OCCURRED)
    {
      pth_sem_dec (&out_signal);
      CArray *c = outqueue.get ();
      t->TracePacket (1, this, "Recv", *c);
      return c;
    }
  else
    return 0;
}

void
BCU1DriverLowLevelDriver::Run (pth_sem_t * stop1)
{
  int i;
  uchar buf[255];
  pth_event_t stop = pth_event (PTH_EVENT_SEM, stop1);
  pth_event_t input = pth_event (PTH_EVENT_SEM, &in_signal);
  while (pth_event_status (stop) != PTH_STATUS_OCCURRED)
    {
      pth_event_concat (stop, input, NULL);
      i = pth_read_ev (fd, buf, sizeof (buf), stop);
      if (i > 0)
	{
	  t->TracePacket (0, this, "Recv", i, buf);
	  outqueue.put (new CArray (buf, i));
	  pth_sem_inc (&out_signal, 1);
	}
      pth_event_isolate (stop);
      if (!inqueue.isempty ())
	{
	  const CArray & c = inqueue.top ();
	  t->TracePacket (0, this, "Send", c);
	  i = pth_write_ev (fd, c.array (), c (), stop);
	  if (i == c ())
	    {
	      pth_sem_dec (&in_signal);
	      inqueue.get ();
	      if (inqueue.isempty ())
		pth_sem_set_value (&send_empty, 1);
	    }
	}
    }
  pth_event_free (stop, PTH_FREE_THIS);
  pth_event_free (input, PTH_FREE_THIS);
}