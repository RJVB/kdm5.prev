/* $TOG: dpylist.c /main/30 1998/02/09 13:55:07 kaleb $ */
/*

Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/
/* $XFree86: xc/programs/xdm/dpylist.c,v 1.3 2000/04/27 16:26:50 eich Exp $ */

/*
 * xdm - display manager daemon
 * Author:  Keith Packard, MIT X Consortium
 *
 * a simple linked list of known displays
 */

#define NEED_SIGNAL
#include "dm.h"
#include "dm_error.h"

struct display	*displays;
static struct disphist	*disphist;

int
AnyDisplaysLeft (void)
{
    return displays != (struct display *) 0;
}

int
AnyActiveDisplays (void)
{
    struct display *d;

Debug("AnyActiveDisplays?\n");
    for (d = displays; d; d = d->next)
	if (d->userSess >= 0)
{Debug(" yes\n");
	    return 1;
}Debug(" no\n");
    return 0;
}

int
AnyRunningDisplays (void)
{
    struct display *d;

Debug("AnyRunningDisplays?\n");
    for (d = displays; d; d = d->next)
	switch (d->status) {
	case notRunning:
	case textMode:
	case reserve:
	    break;
	default:
Debug(" yes\n");
	    return 1;
	}
Debug(" no\n");
    return 0;
}

int
AnyReserveDisplays (void)
{
    struct display *d;

    for (d = displays; d; d = d->next)
	if ((d->displayType & d_lifetime) == dReserve)
	    return 1;
    return 0;
}

#ifdef AUTO_RESERVE
int
AllLocalDisplaysLocked (struct display *dp)
{
    struct display *d;

Debug("AllLocalDisplaysLocked?\n");
    for (d = displays; d; d = d->next)
	if (d != dp &&
	    (d->displayType & d_location) == dLocal &&
	    d->status == running && !d->hstent->lock)
{Debug(" no\n");
	    return 0;
}Debug(" yes\n");
    return 1;
}

void
ReapReserveDisplays (void)
{
    struct display *d, *rd;

Debug("ReapReserveDisplays\n");
    for (rd = 0, d = displays; d; d = d->next)
	if ((d->displayType & d_location) == dLocal && d->status == running &&
	    !d->hstent->lock)
	{
	    if (rd)
	    {
		rd->idleTimeout = 0;
Debug ("killing reserve display %s\n", rd->name);
		if (rd->pid != -1)
		    kill (rd->pid, SIGALRM);
		rd = 0;
	    }
	    if ((d->displayType & d_lifetime) == dReserve &&
		d->userSess < 0)
		rd = d;
	}
}
#endif /* AUTO_RESERVE */

void
StartReserveDisplay (int lt)
{
    struct display *d, *rd;

Debug("StartReserveDisplay\n");
    for (rd = 0, d = displays; d; d = d->next)
	if (d->status == reserve)
	    rd = d;
    if (rd)
    {
Debug("starting reserve display %s, timeout %d\n", rd->name, lt);
	rd->idleTimeout = lt;
	rd->status = notRunning;
    }
}

void
ForEachDisplay (void (*f)(struct display *))
{
    struct display *d, *next;

    for (d = displays; d; d = next) {
	next = d->next;
	(*f) (d);
    }
}

struct display *
FindDisplayByName (const char *name)
{
    struct display *d;

    for (d = displays; d; d = d->next)
	if (!strcmp (name, d->name))
	    return d;
    return 0;
}

struct display *
FindDisplayByPid (int pid)
{
    struct display *d;

    for (d = displays; d; d = d->next)
	if (pid == d->pid)
	    return d;
    return 0;
}

struct display *
FindDisplayByServerPid (int serverPid)
{
    struct display *d;

    for (d = displays; d; d = d->next)
	if (serverPid == d->serverPid)
	    return d;
    return 0;
}

#ifdef XDMCP

struct display *
FindDisplayBySessionID (CARD32 sessionID)
{
    struct display	*d;

    for (d = displays; d; d = d->next)
	if (sessionID == d->sessionID)
	    return d;
    return 0;
}

struct display *
FindDisplayByAddress (XdmcpNetaddr addr, int addrlen, CARD16 displayNumber)
{
    struct display  *d;

    for (d = displays; d; d = d->next)
	if ((d->displayType & d_origin) == dFromXDMCP &&
	    d->displayNumber == displayNumber &&
	    addressEqual ((XdmcpNetaddr)d->from.data, d->from.length, 
			  addr, addrlen))
	    return d;
    return 0;
}

#endif /* XDMCP */

#define IfFree(x)  if (x) free ((char *) x)
    
void
RemoveDisplay (struct display *old)
{
    struct display	*d, **dp;
    int			i;

    for (dp = &displays; (d = *dp); dp = &(*dp)->next) {
	if (d == old) {
Debug ("Removing display %s\n", d->name);
	    *dp = d->next;
	    IfFree (d->class2);
	    IfFree (d->cfg.data);
	    delStr (d->cfg.dep.name);
	    freeStrArr (d->serverArgv);
	    IfFree (d->remoteHost);
	    if (d->authorizations)
	    {
		for (i = 0; i < d->authNum; i++)
		    XauDisposeAuth (d->authorizations[i]);
		free ((char *) d->authorizations);
	    }
	    if (d->authFile) {
		(void) unlink (d->authFile);
		free (d->authFile);
	    }
	    IfFree (d->authNameLens);
#ifdef XDMCP
	    XdmcpDisposeARRAY8 (&d->peer);
	    XdmcpDisposeARRAY8 (&d->from);
	    XdmcpDisposeARRAY8 (&d->clientAddr);
#endif
	    free ((char *) d);
	    break;
	}
    }
}

static struct disphist *
FindHist (const char *name)
{
    struct disphist *hstent;

    for (hstent = disphist; hstent; hstent = hstent->next)
	if (!strcmp (hstent->name, name))
	    return hstent;
    return 0;
}

struct display *
NewDisplay (const char *name)
{
    struct display	*d;
    struct disphist	*hstent;

    if (!(hstent = FindHist (name))) {
	if (!(hstent = Calloc (1, sizeof (*hstent))))
	    return 0;
	if (!StrDup (&hstent->name, name)) {
	    free (hstent);
	    return 0;
	}
	hstent->next = disphist; disphist = hstent;
    }

    if (!(d = (struct display *) Calloc (1, sizeof (*d))))
	return 0;
    d->next = displays;
    d->hstent = hstent;
    d->name = hstent->name;
    /* initialize fields (others are 0) */
    d->pid = -1;
    d->serverPid = -1;
    d->fifofd = -1;
    d->pipe.rfd = -1;
    d->pipe.wfd = -1;
    d->userSess = -1;
    displays = d;
Debug ("created new display %s\n", d->name);
    return d;
}
