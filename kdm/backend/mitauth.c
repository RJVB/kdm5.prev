/* $TOG: mitauth.c /main/13 1998/02/09 13:55:37 kaleb $ */
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
/* $XFree86: xc/programs/xdm/mitauth.c,v 1.2 1998/10/10 15:25:36 dawes Exp $ */

/*
 * xdm - display manager daemon
 * Author:  Keith Packard, MIT X Consortium
 *
 * mitauth
 *
 * generate authorization keys
 * for MIT-MAGIC-COOKIE-1 type authorization
 */

#include "dm.h"
#include "dm_auth.h"

#include <X11/Xos.h>

#define AUTH_DATA_LEN	16	/* bytes of authorization data */
static char	auth_name[256];
static int	auth_name_len;

void
MitInitAuth (unsigned short name_len, const char *name)
{
    if (name_len > 256)
	name_len = 256;
    auth_name_len = name_len;
    memmove( auth_name, name, name_len);
}

Xauth *
MitGetAuth (unsigned short namelen, const char *name)
{
    Xauth   *new;
    new = (Xauth *) Malloc (sizeof (Xauth));

    if (!new)
	return (Xauth *) 0;
    new->family = FamilyWild;
    new->address_length = 0;
    new->address = 0;
    new->number_length = 0;
    new->number = 0;

    new->data = (char *) Malloc (AUTH_DATA_LEN);
    if (!new->data)
    {
	free ((char *) new);
	return (Xauth *) 0;
    }
    new->name = (char *) Malloc (namelen);
    if (!new->name)
    {
	free ((char *) new->data);
	free ((char *) new);
	return (Xauth *) 0;
    }
    memmove( (char *)new->name, name, namelen);
    new->name_length = namelen;
    if (!GenerateAuthData (new->data, AUTH_DATA_LEN))
    {
	free ((char *) new->name);
	free ((char *) new->data);
	free ((char *) new);
	return (Xauth *) 0;
    }
    new->data_length = AUTH_DATA_LEN;
    return new;
}
