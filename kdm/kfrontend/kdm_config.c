    /*

    Read options from kdmrc

    $Id$

    Copyright (C) 2001 Oswald Buddenhagen <ossi@kde.org>


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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    */

#include <config.h>

#include "kdm_config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <grp.h>

#define KDMCONF KDE_CONFDIR "/kdm"


/*
 * Section/Entry definition structs
 */

typedef struct Ent {
	const char *name;
	int id;
	void *ptr;
	const char *def;
} Ent;

typedef struct Sect {
	const char *name;
	Ent *ents;
	int numents;
} Sect;

/*
 * Parsed ini file structs
 */

typedef struct Entry {
	struct Entry *next;
	const char *val;
	int vallen;
	int keyid;
	int line;
} Entry;

typedef struct Section {
	struct Section *next;
	Entry *entries;
	Sect *sect;
	const char *name, *dname, *dhost, *dnum, *dclass;
	int nlen, dlen, dhostl, dnuml, dclassl;
} Section;


/*
 * Split up display-name/-class for fast comparison
 */
typedef struct DSpec {
    const char *dhost, *dnum, *dclass;
    int dhostl, dnuml, dclassl;
} DSpec;


/*
 * Config value storage structures
 */

typedef struct Value {
    const char *ptr;
    int len;
} Value;

typedef struct Val {
    Value val;
    int id;
} Val;

typedef struct ValArr {
    Val *ents;
    int nents, esiz, nchars, nptrs;
} ValArr;


#ifdef HAVE_VSYSLOG
# define USE_SYSLOG
#endif

#define LOG_NAME "kdm_config"
#define LOG_DEBUG_MASK DEBUG_CONFIG
#define LOG_PANIC_EXIT 1
#define STATIC static
#include <printf.c>


static void
MkDSpec (DSpec *spec, const char *dname, const char *dclass)
{
    spec->dhost = dname;
    for (spec->dhostl = 0; dname[spec->dhostl] != ':'; spec->dhostl++);
    spec->dnum = dname + spec->dhostl + 1;
    spec->dnuml = strlen (spec->dnum);
    spec->dclass = dclass;
    spec->dclassl = strlen (dclass);
}


static int rfd, wfd;

static int
Reader (void *buf, int count)
{
    int ret, rlen;

    for (rlen = 0; rlen < count; ) {
      dord:
	ret = read (rfd, (void *)((char *)buf + rlen), count - rlen);
	if (ret < 0) {
	    if (errno == EINTR)
		goto dord;
	    if (errno == EAGAIN)
		break;
	    return -1;
	}
	if (!ret)
	    break;
	rlen += ret;
    }
    return rlen;
}

static void
GRead (void *buf, int count)
{
    if (Reader (buf, count) != count)
	LogPanic ("Can't read from core\n");
}

static void
GWrite (const void *buf, int count)
{
    if (write (wfd, buf, count) != count)
	LogPanic ("Can't write to core\n");
}

static void
GSendInt (int val)
{
    GWrite (&val, sizeof(val));
}

static void
GSendStr (const char *buf)
{
    if (buf) {
	int len = strlen (buf) + 1;
	GWrite (&len, sizeof(len));
	GWrite (buf, len);
    } else
	GWrite (&buf, sizeof(int));
}

static void
GSendNStr (const char *buf, int len)
{
    int tlen = len + 1;
    GWrite (&tlen, sizeof(tlen));
    GWrite (buf, len);
    GWrite ("", 1);
}

static void
GSendArr (int len, const char *data)
{
    GWrite (&len, sizeof(len));
    GWrite (data, len);
}

static int
GRecvCmd (int *val)
{
    if (Reader (val, sizeof(*val)) != sizeof(*val))
	return 0;
    return 1;
}

static int
GRecvInt ()
{
    int val;

    GRead (&val, sizeof(val));
    return val;
}

static char *
GRecvStr ()
{
    int len;
    char *buf;

    len = GRecvInt ();
    if (!len)
	return 0;
    if (!(buf = malloc (len)))
	LogPanic ("No memory for read buffer");
    GRead (buf, len);
    return buf;
}


/* #define WANT_CLOSE 1 */

typedef struct File {
	char *buf, *eof, *cur;
#if defined(HAVE_MMAP) && defined(WANT_CLOSE)
	int ismapped;
#endif
} File;

static int
readFile (File *file, const char *fn, const char *what)
{
    int fd;
    off_t flen;

    if ((fd = open (fn, O_RDONLY)) < 0) {
	LogInfo ("Cannot open %s file %s\n", what, fn);
	return 0;
    }

    flen = lseek (fd, 0, SEEK_END);
#ifdef HAVE_MMAP
# ifdef WANT_CLOSE
    file->ismapped = 0;
# endif
    file->buf = mmap(0, flen + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
# ifdef WANT_CLOSE
    if (file->buf)
	file->ismapped = 1;
    else
# else
    if (!file->buf)
# endif
#endif
    {
	if (!(file->buf = malloc (flen + 1))) {
	    LogOutOfMem ("readFile");
	    close (fd);
	    return 0;
	}
	lseek (fd, 0, SEEK_SET);
	if (read (fd, file->buf, flen) != flen) {
	    free (file->buf);
	    LogError ("Cannot read %s file %s\n", what, fn);
	    close (fd);
	    return 0;
	}
    }
    file->eof = (file->cur = file->buf) + flen;
    close (fd);
    return 1;
}

static int
copyBuf (File *file, const char *buf, int len)
{
    if (!(file->buf = malloc (len + 1))) {
	LogOutOfMem ("copyBuf");
	return 0;
    }
    memcpy (file->buf, buf, len);
    file->eof = (file->cur = file->buf) + len;
#if defined(HAVE_MMAP) && defined(WANT_CLOSE)
    file->ismapped = 0;
#endif
    return 1;
}

#ifdef WANT_CLOSE
static void
freeBuf (File *file)
{
# ifdef HAVE_MMAP
    if (file->ismapped)
	munmap(file->buf, file->eof - file->buf + 1);
    else
# endif
	free (file->buf);
}
#endif

static int daemonize = -1, autolog = 1;
static Value VnoPassEnable, VautoLoginEnable, VxdmcpEnable,
	VXaccess, VXservers;

#define C_MTYPE_MASK	0x30000000
# define C_PATH		0x10000000	/* C_TYPE_STR is a path spec */
# define C_BOOL		0x10000000	/* C_TYPE_INT is a boolean */
# define C_ENUM		0x20000000	/* C_TYPE_INT is an enum (option) */
# define C_GRP		0x30000000	/* C_TYPE_INT is a group spec */
#define C_INTERNAL	0x40000000	/* don't expose to core */
#define C_CONFIG	0x80000000	/* process only for finding deps */

#define C_noPassEnable		( C_TYPE_INT | C_INTERNAL | 0x2000 )
#define C_autoLoginEnable	( C_TYPE_INT | C_INTERNAL | 0x2001 )
#define C_xdmcpEnable		( C_TYPE_INT | C_INTERNAL | 0x2002 )
#define C_accessFile		( C_TYPE_STR | C_INTERNAL | C_CONFIG | 0x2003 )
#define C_servers		( C_TYPE_STR | C_INTERNAL | C_CONFIG | 0x2004 )

static int
PdaemonMode (Value *retval)
{
    if (daemonize >= 0) {
	retval->ptr = (char *)daemonize;
	return 1;
    }
    return 0;
}

static int
PautoLogin (Value *retval)
{
    retval->ptr = (char *)autolog;
    return 1;
}

static int
PrequestPort (Value *retval)
{
    if (!VxdmcpEnable.ptr) {
	retval->ptr = (char *)0;
	return 1;
    }
    return 0;
}

static Value
    emptyStr = { "", 1 },
    nullValue = { 0, 0 }, 
    emptyArgv = { (char *)&nullValue, 0 };

static int
PnoPassUsers (Value *retval)
{
    if (!VnoPassEnable.ptr) {
	*retval = emptyArgv;
	return 1;
    }
    return 0;
}

static int
PautoLoginX (Value *retval)
{
    if (!VautoLoginEnable.ptr) {
	*retval = emptyStr;
	return 1;
    }
    return 0;
}

#ifndef HALT_CMD
# ifdef BSD
#  define HALT_CMD	"/sbin/shutdown -h now"
#  define REBOOT_CMD	"/sbin/shutdown -r now"
# elif defined(__SVR4)
#  define HALT_CMD	"/usr/sbin/halt"
#  define REBOOT_CMD	"/usr/sbin/reboot"
# else
#  define HALT_CMD	"/sbin/halt"
#  define REBOOT_CMD	"/sbin/reboot"
# endif
#endif

#ifndef DEF_USER_PATH
#  if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__linux__)
#    define DEF_USER_PATH "/bin:/usr/bin:" XBINDIR ":/usr/local/bin"
#  else
#    define DEF_USER_PATH "/bin:/usr/bin:" XBINDIR ":/usr/local/bin:/usr/ucb"
#  endif
#endif
#ifndef DEF_SYSTEM_PATH
#  if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__linux__)
#    define DEF_SYSTEM_PATH "/sbin:/usr/sbin:/bin:/usr/bin:" XBINDIR ":/usr/local/bin"
#  else
#    define DEF_SYSTEM_PATH "/sbin:/usr/sbin:/bin:/usr/bin:" XBINDIR ":/usr/local/bin:/etc:/usr/ucb"
#  endif
#endif

#ifndef DEF_AUTH_NAME
# ifdef HASXDMAUTH
#  define DEF_AUTH_NAME	"XDM-AUTHORIZATION-1,MIT-MAGIC-COOKIE-1"
# else
#  define DEF_AUTH_NAME	"MIT-MAGIC-COOKIE-1"
# endif
#endif

#ifdef __linux__
# define DEF_SERVER_LINE ":0 local@tty1 " XBINDIR "/X vt7"
#elif defined(sun)
# define DEF_SERVER_LINE ":0 local@console " XBINDIR "/X"
#elif defined(_AIX)
# define DEF_SERVER_LINE ":0 local@lft0 " XBINDIR "/X"
#else
# define DEF_SERVER_LINE ":0 local " XBINDIR "/X"
#endif

static const char
    *logoarea[] = { "None", "Logo", "Clock", 0 },
    *showusers[] = { "NotHidden", "Selected", "None", 0 },
    *preseluser[] = { "None", "Previous", "Default", 0 },
    *echomode[] = { "OneStar", "ThreeStars", "NoEcho", 0 },
    *sd_who[] = { "None", "Root", "All", 0 },
    *sd_mode[] = { "Schedule", "TryNow", "ForceNow", 0 };

Ent entsGeneral[] = {
{ "DaemonMode",		C_daemonMode | C_BOOL,	(void *)PdaemonMode,	"true" },
{ "Xservers",		C_servers,		&VXservers,	DEF_SERVER_LINE },
{ "PidFile",		C_pidFile,		0,	"" },
{ "LockPidFile",	C_lockPidFile | C_BOOL,	0,	"true" },
{ "AuthDir",		C_authDir | C_PATH,	0,	"/var/run/xauth" },
{ "AutoRescan",		C_autoRescan | C_BOOL,	0,	"true" },
{ "ExportList",		C_exportList,		0,	"" },
#if !defined(__linux__) && !defined(__OpenBSD__)
{ "RandomFile",		C_randomFile,		0,	"/dev/mem" },
#endif
{ "AutoLogin",		C_autoLogin | C_BOOL,	(void *)PautoLogin,	"true" },
{ "FifoDir",		C_fifoDir | C_PATH,	0,	"/var/run/xdmctl" },
{ "FifoGroup",		C_fifoGroup | C_GRP,	0,	"0" },
};

Ent entsXdmcp[] = {
{ "Enable",		C_xdmcpEnable | C_BOOL,	&VxdmcpEnable,	"true" },
{ "Port",		C_requestPort,		(void *)PrequestPort,	"177" },
{ "KeyFile",		C_keyFile,		0,	KDMCONF "/kdmkeys" },
{ "Xaccess",		C_accessFile,		&VXaccess,	KDMCONF "/Xaccess" },
{ "ChoiceTimeout",	C_choiceTimeout,	0,	"15" },
{ "RemoveDomainname",	C_removeDomainname | C_BOOL, 0,	"true" },
{ "SourceAddress",	C_sourceAddress | C_BOOL, 0,	"false" },
{ "Willing",		C_willing,		0,	"" },
};

Ent entsShutdown[] = {
{ "HaltCmd",		C_cmdHalt,		0,	HALT_CMD },
{ "RebootCmd",		C_cmdReboot,		0,	REBOOT_CMD },
{ "AllowFifo",		C_fifoAllowShutdown | C_BOOL,	0,	"false" },
{ "AllowFifoNow",	C_fifoAllowNuke | C_BOOL,	0,	"true" },
#ifdef __linux__
{ "UseLilo",		C_useLilo | C_BOOL,	0,	"false" },
{ "LiloCmd",		C_liloCmd,		0,	"/sbin/lilo" },
{ "LiloMap",		C_liloMap,		0,	"/boot/map" },
#endif
};

Ent entsCore[] = {
{ "ServerAttempts",	C_serverAttempts,	0,	"1" },
{ "ServerTimeout",	C_serverTimeout,	0,	"15" },
{ "OpenDelay",		C_openDelay,		0,	"15" },
{ "OpenRepeat",		C_openRepeat,		0,	"5" },
{ "OpenTimeout",	C_openTimeout,		0,	"120" },
{ "StartAttempts",	C_startAttempts,	0,	"4" },
{ "PingInterval",	C_pingInterval,		0,	"5" },
{ "PingTimeout",	C_pingTimeout,		0,	"5" },
{ "TerminateServer",	C_terminateServer | C_BOOL, 0,	"false" },
{ "ResetSignal",	C_resetSignal,		0,	"1" },	/* SIGHUP */
{ "TermSignal",		C_termSignal,		0,	"15" },	/* SIGTERM */
{ "ResetForAuth",	C_resetForAuth | C_BOOL, 0,	"false" },
{ "Authorize",		C_authorize | C_BOOL,	0,	"true" },
{ "AuthNames",		C_authNames,		0,	DEF_AUTH_NAME },
{ "AuthFile",		C_clientAuthFile,	0,	"" },
{ "StartInterval",	C_startInterval,	0,	"30" },
{ "Resources",		C_resources,		0,	"" },
{ "Xrdb",		C_xrdb,			0,	XBINDIR "/xrdb" },
{ "Setup",		C_setup,		0,	"" },
{ "Startup",		C_startup,		0,	"" },
{ "Reset",		C_reset,		0,	"" },
{ "Session",		C_session,		0,	XBINDIR "/xterm -ls -T" },
{ "UserPath",		C_userPath,		0,	DEF_USER_PATH },
{ "SystemPath",		C_systemPath,		0,	DEF_SYSTEM_PATH },
{ "SystemShell",	C_systemShell,		0,	"/bin/sh" },
{ "FailsafeClient",	C_failsafeClient,	0,	XBINDIR "/xterm" },
{ "UserAuthDir",	C_userAuthDir | C_PATH,	0,	"/tmp" },
{ "Chooser",		C_chooser,		0,	KDE_BINDIR "/chooser" },
{ "NoPassEnable",	C_noPassEnable | C_BOOL, &VnoPassEnable, "false" },
{ "NoPassUsers",	C_noPassUsers,	(void *)PnoPassUsers,	"" },
{ "AutoLoginEnable",	C_autoLoginEnable | C_BOOL, &VautoLoginEnable, "false" },
{ "AutoLoginUser",	C_autoUser,	(void *)PautoLoginX,	"" },
{ "AutoLoginPass",	C_autoPass,	(void *)PautoLoginX,	"" },
{ "AutoLoginSession",	C_autoString,	(void *)PautoLoginX,	"" },
{ "AutoLogin1st",	C_autoLogin1st | C_BOOL, 0,	"true" },
{ "AutoReLogin",	C_autoReLogin | C_BOOL,	0,	"false" },
{ "AllowNullPasswd",	C_allowNullPasswd | C_BOOL, 0,	"true" },
{ "AllowRootLogin",	C_allowRootLogin | C_BOOL, 0,	"true" },
{ "SessSaveFile",	C_sessSaveFile,		0,	".wmrc" },
{ "AllowShutdown",	C_allowShutdown | C_ENUM, sd_who, "All" },
{ "AllowSdForceNow",	C_allowNuke | C_ENUM, sd_who,	"All" },
{ "DefaultSdMode",	C_defSdMode | C_ENUM, sd_mode,	"Schedule" },
};

Ent entsGreeter[] = {
{ "SessionTypes",	C_SessionTypes,		0,	"default,failsafe" },
{ "GUIStyle",		C_GUIStyle, 		0,	"KDE" },
{ "LogoArea",		C_LogoArea | C_ENUM, logoarea,	"Logo" },
{ "LogoPixmap",		C_LogoPixmap,		0,	"" },
{ "GreeterPosFixed",	C_GreeterPosFixed | C_BOOL, 0,	"false" },
{ "GreeterPosX",	C_GreeterPosX,		0,	"0" },
{ "GreeterPosY",	C_GreeterPosY,		0,	"0" },
{ "GreeterScreen",	C_GreeterScreen,	0,	"0" },
{ "StdFont",		C_StdFont,		0,	"helvetica,12,5,0,50,0" },
{ "FailFont",		C_FailFont,		0,	"helvetica,12,5,0,75,0" },
{ "GreetString",	C_GreetString,		0,	"Welcome to %s at %n" },
{ "GreetFont",		C_GreetFont,		0,	"charter,24,5,0,50,0" },
{ "AntiAliasing",	C_AntiAliasing,		0,	"false" },
{ "Language",		C_Language,		0,	"en_US" },
{ "ShowUsers",		C_ShowUsers | C_ENUM, showusers, "NotHidden" },
{ "SelectedUsers",	C_SelectedUsers,	0,	"" },
{ "HiddenUsers",	C_HiddenUsers,		0,	"" },
{ "MinShowUID",		C_MinShowUID,		0,	"0" },
{ "MaxShowUID",		C_MaxShowUID,		0,	"65535" },
{ "SortUsers",		C_SortUsers | C_BOOL,	0,	"true" },
{ "PreselectUser",	C_PreselectUser | C_ENUM, preseluser, "None" },
{ "DefaultUser",	C_DefaultUser,		0,	"" },
{ "FocusPasswd",	C_FocusPasswd | C_BOOL, 0,	"false" },
{ "EchoMode",		C_EchoMode | C_ENUM, echomode,	"OneStar" },
{ "GrabServer",		C_grabServer | C_BOOL,	0,	"false" },
{ "GrabTimeout",	C_grabTimeout,		0,	"3" },
{ "AuthComplain",	C_authComplain | C_BOOL, 0,	"true" },
};

Ent entsDesktop[] = {
{ "BackgroundMode",	C_BackgroundMode,	0,	"VerticalGradient" },
{ "BlendBalance",	C_BlendBalance,		0,	"100" },
{ "BlendMode",		C_BlendMode,		0,	"NoBlending" },
{ "ChangeInterval",	C_ChangeInterval,	0,	"60" },
{ "Color1",		C_Color1,		0,	"30,114,160" },
{ "Color2",		C_Color2,		0,	"192,192,192" },
{ "CurrentWallpaper",	C_CurrentWallpaper,	0,	"0" },
{ "LastChange",		C_LastChange,		0,	"0" },
{ "MinOptimizationDepth", C_MinOptimizationDepth, 0,	"1" },
{ "MultiWallpaperMode",	C_MultiWallpaperMode,	0,	"NoMulti" },
{ "Pattern",		C_Pattern,		0,	"" },
{ "Program",		C_Program,		0,	"" },
{ "ReverseBlending",	C_ReverseBlending | C_BOOL, 0,	"false" },
{ "UseSHM",		C_UseSHM | C_BOOL,	0,	"false" },
{ "Wallpaper",		C_Wallpaper,		0,	"" },
{ "WallpaperList",	C_WallpaperList,	0,	"" },
{ "WallpaperMode",	C_WallpaperMode,	0,	"Tiled" },
};

/* Don't change order! */
Sect allSects[] = { 
 { "General", entsGeneral, as(entsGeneral) },
 { "Xdmcp", entsXdmcp, as(entsXdmcp) },
 { "Shutdown", entsShutdown, as(entsShutdown) },
 { "Desktop0", entsDesktop, as(entsDesktop) }, /* XXX should be -Desktop */
 { "-Core", entsCore, as(entsCore) },
 { "-Greeter", entsGreeter, as(entsGreeter) },
};

static const char *kdmrc = KDMCONF "/kdmrc";

static Section *rootsec;

static void
ReadConf ()
{
    const char *nstr, *dstr, *cstr, *dhost, *dnum, *dclass;
    char *s, *e, *st, *en, *ek, *sl, *pt;
    Section *cursec;
    Entry *curent;
    int nlen, dlen, clen, dhostl, dnuml, dclassl;
    int i, line, sectmoan, restl;
    File file;
    static int confread;

    if (confread)
	return;
    confread = 1;

Debug ("reading config %s ...\n", kdmrc);
    if (!readFile (&file, kdmrc, "master configuration"))
	return;

Debug ("parsing config ...\n");
    for (s = file.buf, line = 0, cursec = 0, sectmoan = 1; s < file.eof; s++) {
	line++;

	while ((s < file.eof) && isspace (*s) && (*s != '\n'))
	    s++;

	if ((s < file.eof) && ((*s == '\n') || (*s == '#'))) {
	  sktoeol:
	    while ((s < file.eof) && (*s != '\n'))
		s++;
	    continue;
	}
	sl = s;

	if (*s == '[') {
	    while ((s < file.eof) && (*s != '\n'))
		s++;
	    e = s - 1;
	    while ((e > sl) && isspace (*e))
		e--;
	    if (*e != ']') {
		LogError ("Invalid section header at %s:%d\n", kdmrc, line);
		continue;
	    }
	    sectmoan = 0;
	    nstr = sl + 1;
	    nlen = e - nstr;
	    for (cursec = rootsec; cursec; cursec = cursec->next)
		if (nlen == cursec->nlen && 
		    !memcmp (nstr, cursec->name, nlen))
		{
		    LogInfo ("Multiple occurences of section [%.*s] in %s. "
			     "Consider merging them.\n", nlen, nstr, kdmrc);
		    goto secfnd;
		}
	    if (nstr[0] == 'X' && nstr[1] == '-') {
		for (dstr = nstr + 2, dlen = 0; ; dlen++) {
		    if (dlen + 2 >= nlen)
			goto illsec;
		    if (dstr[dlen] == '-')
			break;
		}
		dhost = dstr;
		dhostl = 0;
		for (restl = dlen; restl; restl--) {
		    if (dhost[dhostl] == ':') {
			dnum = dhost + dhostl + 1;
			dnuml = 0;
			for (restl--; restl; restl--) {
			    if (dnum[dnuml] == '_') {
				dclass = dnum + dnuml + 1;
				dclassl = restl;
				goto gotall;
			    }
			    dnuml++;
			}
			goto gotnum;
		    }
		    dhostl++;
		}
		dnum = "*";
		dnuml = 1;
	      gotnum:
		dclass = "*";
		dclassl = 1;
	      gotall:
		cstr = dstr + dlen;
		clen = nlen - dlen - 2;
	    } else {
		if (nstr[0] == '-')
		    goto illsec;
		dstr = 0;
		dlen = 0;
		dhost = 0;
		dhostl = 0;
		dnum = 0;
		dnuml = 0;
		dclass = 0;
		dclassl = 0;
		cstr = nstr;
		clen = nlen;
	    }
	    for (i = 0; i < as(allSects); i++)
		if ((int)strlen (allSects[i].name) == clen && 
		    !memcmp (allSects[i].name, cstr, clen))
		    goto newsec;
	  illsec:
	    cursec = 0;
	    LogError ("Unrecognized section name [%.*s] at %s:%d\n", 
		      nlen, nstr, kdmrc, line);
	    continue;
	  newsec:
	    if (!(cursec = malloc (sizeof(*cursec)))) {
		LogOutOfMem ("ReadConf");
		return;
	    }
	    cursec->name = nstr;
	    cursec->nlen = nlen;
	    cursec->dname = dstr;
	    cursec->dlen = dlen;
	    cursec->dhost = dhost;
	    cursec->dhostl = dhostl;
	    cursec->dnum = dnum;
	    cursec->dnuml = dnuml;
	    cursec->dclass = dclass;
	    cursec->dclassl = dclassl;
	    cursec->sect = allSects + i;
	    cursec->entries = 0;
	    cursec->next = rootsec;
	    rootsec = cursec;
	    Debug ("now in section [%.*s], dpy '%.*s', core '%.*s'\n", 
		   nlen, nstr, dlen, dstr, clen, cstr);
	  secfnd:
	    continue;
	}

	if (!cursec) {
	    if (sectmoan) {
		sectmoan = 0;
		LogError ("Entry outside any section at %s:%d", kdmrc, line);
	    }
	    goto sktoeol;
	}

	for (; (s < file.eof) && (*s != '\n'); s++)
	    if (*s == '=')
		goto haveeq;
	LogError ("Invalid entry (missing '=') at %s:%d\n", kdmrc, line);
	continue;

      haveeq:
	for (ek = s - 1; ; ek--) {
	    if (ek < sl) {
		LogError ("Invalid entry (empty key) at %s:%d\n", kdmrc, line);
		goto sktoeol;
	    }
	    if (!isspace (*ek))
		break;
	}

	s++;
	while ((s < file.eof) && isspace(*s) && (*s != '\n'))
	    s++;
	for (pt = st = en = s; s < file.eof && *s != '\n'; s++) {
	    if (*s == '\\') {
		s++;
		if (s >= file.eof || *s == '\n') {
		    LogError ("Trailing backslash at %s:%d\n", kdmrc, line);
		    break;
		}
		switch(*s) {
		case 's': *pt++ = ' '; break;
		case 't': *pt++ = '\t'; break;
		case 'n': *pt++ = '\n'; break;
		case 'r': *pt++ = '\r'; break;
		case '\\': *pt++ = '\\'; break;
		default: 
		    LogError ("Unrecognized escape '\\%c' at %s:%d\n", 
			      *s, kdmrc, line);
		    break;
		}
		en = pt;
	    } else {
		*pt++ = *s;
		if (*s != ' ' && *s != '\t')
		    en = pt;
	    }
        }

	nstr = sl;
	nlen = ek - sl + 1;
	for (i = 0; i < cursec->sect->numents; i++)
	    if ((int)strlen (cursec->sect->ents[i].name) == nlen && 
		!memcmp (cursec->sect->ents[i].name, nstr, nlen))
		goto keyok;
	LogError ("Unrecognized key '%.*s' at %s:%d\n", 
		  nlen, nstr, kdmrc, line);
	goto keyfnd;
      keyok:
	for (curent = cursec->entries; curent; curent = curent->next)
	    if (cursec->sect->ents[i].id == curent->keyid) {
		LogError ("Multiple occurences of key '%.*s' in section [%.*s]"
			  " of %s.\n", 
			  nlen, nstr, cursec->nlen, cursec->name, kdmrc);
		goto keyfnd;
	    }
	if (!(curent = malloc (sizeof (*curent)))) {
	    LogOutOfMem ("ReadConf");
	    return;
	}
	curent->keyid = cursec->sect->ents[i].id;
	curent->line = line;
	curent->val = st;
	curent->vallen = en - st;
	curent->next = cursec->entries;
	cursec->entries = curent;
      keyfnd:
	Debug ("read entry '%.*s'='%.*s'\n", nlen, nstr, en - st, st);
	continue;
    }
Debug ("config parsed\n");
}

static Entry *
FindGEnt (int id)
{
    Section *cursec;
    Entry *curent;

    for (cursec = rootsec; cursec; cursec = cursec->next)
	if (!cursec->dname)
	    for (curent = cursec->entries; curent; curent = curent->next)
		if (curent->keyid == id)
		    return curent;
    return 0;
}

/* Display name match scoring:
 * - class (any/exact) -> 0/1
 * - number (any/exact) -> 0/2
 * - host (any/trail/wild/exact) -> 0/4/8/12
 */
static Entry *
FindDEnt (int id, DSpec *dspec)
{
    Section *cursec;
    Entry *curent, *bestent;
    int score, bestscore;

    bestscore = -1, bestent = 0;
    for (cursec = rootsec; cursec; cursec = cursec->next)
	if (cursec->dname) {
	    score = 0;
	    if (cursec->dclassl != 1 || cursec->dclass[0] != '*') {
		if (cursec->dclassl == dspec->dclassl && 
		    !memcmp (cursec->dclass, dspec->dclass, dspec->dclassl))
		    score = 1;
		else
		    continue;
	    }
	    if (cursec->dnuml != 1 || cursec->dnum[0] != '*') {
		if (cursec->dnuml == dspec->dnuml && 
		    !memcmp (cursec->dnum, dspec->dnum, dspec->dnuml))
		    score += 2;
		else
		    continue;
	    }
	    if (cursec->dhostl != 1 || cursec->dhost[0] != '*') {
		if (cursec->dhost[0] == '.') {
		    if (cursec->dhostl < dspec->dhostl && 
			!memcmp (cursec->dhost, 
				 dspec->dhost + dspec->dhostl - cursec->dhostl, 
				 cursec->dhostl))
			score += 4;
		    else
			continue;
		} else {
		    if (cursec->dhostl == dspec->dhostl && 
			!memcmp (cursec->dhost, dspec->dhost, dspec->dhostl))
			score += 12;
		    else
			continue;
		}
	    }
	    if (score > bestscore) {
		for (curent = cursec->entries; curent; curent = curent->next)
		    if (curent->keyid == id) {
			bestent = curent;
			bestscore = score;
			break;
		    }
	    }
	}
    return bestent;
}

static const char *
CvtValue (Ent *et, Value *retval, int vallen, const char *val, char **eopts)
{
    Value *ents;
    int i, b, e, tlen, nents, esiz;
    char buf[80];

    switch (et->id & C_TYPE_MASK) {
	case C_TYPE_INT:
	    for (i = 0; i < vallen && i < (int)sizeof(buf) - 1; i++)
		buf[i] = tolower (val[i]);
	    buf[i] = 0;
	    if ((et->id & C_MTYPE_MASK) == C_BOOL) {
		if (!strcmp (buf, "true") ||
		    !strcmp (buf, "on") ||
		    !strcmp (buf, "yes") ||
		    !strcmp (buf, "1"))
			retval->ptr = (char *)1;
		else if (!strcmp (buf, "false") ||
			 !strcmp (buf, "off") ||
			 !strcmp (buf, "no") ||
			 !strcmp (buf, "0"))
			    retval->ptr = (char *)0;
		else
		    return "boolean";
		return 0;
	    } else if ((et->id & C_MTYPE_MASK) == C_ENUM) {
		for (i = 0; eopts[i]; i++)
		    if (!memcmp (eopts[i], val, vallen) && !eopts[i][vallen]) {
			retval->ptr = (char *)i;
			return 0;
		    }
		return "option";
	    } else if ((et->id & C_MTYPE_MASK) == C_GRP) {
		struct group *ge;
		if ((ge = getgrnam (buf))) {
		    retval->ptr = (char *)ge->gr_gid;
		    return 0;
		}
	    }
	    retval->ptr = 0;
	    if (sscanf (buf, "%i", (int *)&retval->ptr) != 1)
		return "integer";
	    return 0;
	case C_TYPE_STR:
	    retval->ptr = val;
	    retval->len = vallen + 1;
	    if ((et->id & C_MTYPE_MASK) == C_PATH)
		if (vallen && val[vallen-1] == '/')
		    retval->len--;
	    return 0;
	case C_TYPE_ARGV:
	    if (!(ents = malloc (sizeof(Value) * (esiz = 10)))) {
		LogOutOfMem ("CvtValue");
		return 0;
	    }
	    for (nents = 0, tlen = 0, i = 0; ; i++) {
		for (; i < vallen && isspace (val[i]); i++);
		for (b = i; i < vallen && val[i] != ','; i++);
		if (b == i)
		    break;
		for (e = i; e > b && isspace (val[e - 1]); e--);
		if (esiz < nents + 2) {
		    Value *entsn = realloc (ents, 
					sizeof(Value) * (esiz = esiz * 2 + 1));
		    if (!nents) {
			LogOutOfMem ("CvtValue");
			break;
		    } else
			ents = entsn;
		}
		ents[nents].ptr = val + b;
		ents[nents].len = e - b;
		nents++;
		tlen += e - b + 1;
	    }
	    ents[nents].ptr = 0;
	    retval->ptr = (char *)ents;
	    retval->len = tlen;
	    return 0;
	default:
	    LogError ("Internal error: unknown value type in id 0x%x\n", et->id);
	    return 0;
    }
}

static void
GetValue (Ent *et, DSpec *dspec, Value *retval, char **eopts)
{
    Entry *ent;
    const char *errs;

/*    Debug ("Getting value 0x%x\n", et->id);*/
    if (dspec)
	ent = FindDEnt (et->id, dspec);
    else
	ent = FindGEnt (et->id);
    if (ent) {
	if (!(errs = CvtValue (et, retval, ent->vallen, ent->val, eopts)))
	    return;
	LogError ("Invalid %s value '%.*s' at %s:%d\n", 
		  errs, ent->vallen, ent->val, kdmrc, ent->line);
    }
    if ((errs = CvtValue (et, retval, strlen (et->def), et->def, eopts)))
	LogError ("Internal error: invalid default %s value '%s' for key %s\n", 
		  errs, et->def, et->name);
}

static int
AddValue (ValArr *va, int id, Value *val)
{
    int nu;

/*    Debug ("Addig value 0x%x\n", id);*/
    if (va->nents == va->esiz) {
	va->ents = realloc (va->ents, sizeof(Val) * (va->esiz += 50));
	if (!va->ents) {
	    LogOutOfMem ("AddValue");
	    return 0;
	}
    }
    va->ents[va->nents].id = id;
    va->ents[va->nents].val = *val;
    va->nents++;
    switch (id & C_TYPE_MASK) {
	case C_TYPE_INT:
	    break;
	case C_TYPE_STR:
	    va->nchars += val->len;
	    break;
	case C_TYPE_ARGV:
	    va->nchars += val->len;
	    for (nu = 0; ((Value *)val->ptr)[nu++].ptr; );
	    va->nptrs += nu;
	    break;
    }
    return 1;
}

static void
CopyValues (ValArr *va, Sect *sec, DSpec *dspec, int isconfig)
{
    Value val;
    int i;

/*Debug ("copying values from section [%s]\n", sec->name);*/
    for (i = 0; i < sec->numents; i++) {
/*Debug ("value 0x%x\n", sec->ents[i].id);*/
	if ((sec->ents[i].id & (int)C_CONFIG) != isconfig)
	    ;
	else if (sec->ents[i].id & C_INTERNAL) {
	    GetValue (sec->ents + i, dspec, ((Value *)sec->ents[i].ptr), 0);
	} else {
	    if (((sec->ents[i].id & C_MTYPE_MASK) == C_ENUM) || 
		!sec->ents[i].ptr ||
		!((int (*)(Value *))sec->ents[i].ptr)(&val)) {
		GetValue (sec->ents + i, dspec, &val, 
			  (char **)sec->ents[i].ptr);
	    }
	    if (!AddValue (va, sec->ents[i].id, &val))
		break;
	}
    }
    return;
}

static void
SendValues (ValArr *va)
{
    Value *cst;
    int i, nu;

Debug ("sending values\n");
    GSendInt (va->nents);
    GSendInt (va->nptrs);
    GSendInt (0/*va->nints*/);
    GSendInt (va->nchars);
    for (i = 0; i < va->nents; i++) {
	GSendInt (va->ents[i].id & ~C_PRIVATE);
	switch (va->ents[i].id & C_TYPE_MASK) {
	case C_TYPE_INT:
	    GSendInt ((int)va->ents[i].val.ptr);
	    break;
	case C_TYPE_STR:
	    GSendNStr (va->ents[i].val.ptr, va->ents[i].val.len - 1);
	    break;
	case C_TYPE_ARGV:
	    cst = (Value *)va->ents[i].val.ptr;
	    for (nu = 0; cst[nu].ptr; nu++);
	    GSendInt (nu);
	    for (; cst->ptr; cst++)
		GSendNStr (cst->ptr, cst->len);
	    break;
	}
    }
Debug ("values sent\n");
}


static char *
ReadWord (File *file, int *len, int EOFatEOL)
{
    char    *wordp, *wordBuffer;
    int	    quoted;
    char    c;

  rest:
    wordp = wordBuffer = file->cur;
  mloop:
    quoted = 0;
  qloop:
    if (file->cur == file->eof)
    {
      doeow:
	if (wordp == wordBuffer)
	    return 0;
      retw:
	*wordp = '\0';
	*len = wordp - wordBuffer;
	return wordBuffer;
    }
    c = *file->cur++;
    switch (c) {
    case '#':
	if (quoted)
	    break;
	do {
	    if (file->cur == file->eof)
		goto doeow;
	    c = *file->cur++;
	} while (c != '\n');
    case '\0':
    case '\n':
	if (EOFatEOL && !quoted)
	{
	    file->cur--;
	    goto doeow;
	}
	if (wordp != wordBuffer)
	{
	    file->cur--;
	    goto retw;
	}
	goto rest;
    case ' ':
    case '\t':
	if (wordp != wordBuffer)
	    goto retw;
	goto rest;
    case '\\':
	if (!quoted)
	{
	    quoted = 1;
	    goto qloop;
	}
	break;
    }
    *wordp++ = c;
    goto mloop;
}


#define ALIAS_CHARACTER	    '%'
#define NEGATE_CHARACTER    '!'
#define CHOOSER_STRING	    "CHOOSER"
#define BROADCAST_STRING    "BROADCAST"
#define NOBROADCAST_STRING  "NOBROADCAST"

typedef struct hostEntry {
    struct hostEntry	*next;
    int		type;
    union _hostOrAlias {
	char	*aliasPattern;
	char	*hostPattern;
	struct _display {
	    int		connectionType;
	    int		hostAddrLen;
	    char	*hostAddress;
	} displayAddress;
    } entry;
} HostEntry;

typedef struct aliasEntry {
    struct aliasEntry	*next;
    char	*name;
    int		hosts;
    int		nhosts;
} AliasEntry;

typedef struct aclEntry {
    struct aclEntry	*next;
    int		entries;
    int		nentries;
    int		hosts;
    int		nhosts;
    int		flags;
} AclEntry;


static int
HasGlobCharacters (char *s)
{
    for (;;)
	switch (*s++) {
	case '?':
	case '*':
	    return 1;
	case '\0':
	    return 0;
	}
}

static int
ParseHost (int *nHosts, HostEntry ***hostPtr, int *nChars, 
	   char *hostOrAlias, int len)
{
    struct hostent  *hostent;

    if (!(**hostPtr = (HostEntry *) malloc (sizeof (HostEntry))))
    {
	LogOutOfMem ("ParseHost");
	return 0;
    }
    if (!strcmp (hostOrAlias, BROADCAST_STRING))
    {
	(**hostPtr)->type = HOST_BROADCAST;
    }
    else if (*hostOrAlias == ALIAS_CHARACTER)
    {
	(**hostPtr)->type = HOST_ALIAS;
	(**hostPtr)->entry.aliasPattern = hostOrAlias + 1;
	*nChars += len;
    }
    else if (HasGlobCharacters (hostOrAlias))
    {
	(**hostPtr)->type = HOST_PATTERN;
	(**hostPtr)->entry.hostPattern = hostOrAlias;
	*nChars += len + 1;
    }
    else
    {
	(**hostPtr)->type = HOST_ADDRESS;
	if (!(hostent = gethostbyname (hostOrAlias)))
	{
	    LogError ("Host \"%s\" not found\n", hostOrAlias);
	    free ((char *) (**hostPtr));
	    return 0;
	}
	if (!((**hostPtr)->entry.displayAddress.hostAddress = 
	      malloc (hostent->h_length)))
	{
	    LogOutOfMem ("ParseHost");
	    free ((char *) (**hostPtr));
	    return 0;
	}
	memcpy ((**hostPtr)->entry.displayAddress.hostAddress, 
		hostent->h_addr, hostent->h_length);
	*nChars += hostent->h_length;
	(**hostPtr)->entry.displayAddress.hostAddrLen = hostent->h_length;
	(**hostPtr)->entry.displayAddress.connectionType = hostent->h_addrtype;
    }
    *hostPtr = &(**hostPtr)->next;
    (*nHosts)++;
    return 1;
}

static void
ReadAccessFile (const char *fname)
{
    HostEntry		*hostList, **hostPtr = &hostList;
    AliasEntry		*aliasList, **aliasPtr = &aliasList;
    AclEntry		*acList, **acPtr = &acList;
    char		*displayOrAlias, *hostOrAlias;
    File		file;
    int			nHosts, nAliases, nAcls, nChars, error;
    int			i, len;

    nHosts = nAliases = nAcls = nChars = error = 0;
    if (!readFile (&file, fname, "XDMCP access control"))
	goto sendacl;
    while ((displayOrAlias = ReadWord (&file, &len, FALSE)))
    {
	if (*displayOrAlias == ALIAS_CHARACTER)
	{
	    if (!(*aliasPtr = (AliasEntry *) malloc (sizeof (AliasEntry))))
	    {
	      oom:
		LogOutOfMem ("ReadAccessFile");
		error = 1;
		break;
	    }
	    (*aliasPtr)->name = displayOrAlias + 1;
	    nChars += len;
	    (*aliasPtr)->hosts = nHosts;
	    (*aliasPtr)->nhosts = 0;
	    while ((hostOrAlias = ReadWord (&file, &len, TRUE)))
	    {
		if (!ParseHost (&nHosts, &hostPtr, &nChars, hostOrAlias, len))
		    goto sktoeol;
		(*aliasPtr)->nhosts++;
	    }
	    aliasPtr = &(*aliasPtr)->next;
	    nAliases++;
	}
	else
	{
	    if (!(*acPtr = (AclEntry *) malloc (sizeof (AclEntry))))
		goto oom;
	    (*acPtr)->flags = 0;
	    if (*displayOrAlias == NEGATE_CHARACTER)
	    {
		(*acPtr)->flags |= a_notAllowed;
		displayOrAlias++;
	    }
	    (*acPtr)->entries = nHosts;
	    (*acPtr)->nentries = 1;
	    if (!ParseHost (&nHosts, &hostPtr, &nChars, displayOrAlias, len))
	    {
	      sktoeol:
		while (ReadWord (&file, &len, TRUE));
		error = 1;
		continue;
	    }
	    (*acPtr)->hosts = nHosts;
	    (*acPtr)->nhosts = 0;
	    while ((hostOrAlias = ReadWord (&file, &len, TRUE)))
	    {
		if (!strcmp (hostOrAlias, CHOOSER_STRING))
		    (*acPtr)->flags |= a_useChooser;
		else if (!strcmp (hostOrAlias, NOBROADCAST_STRING))
		    (*acPtr)->flags |= a_notBroadcast;
		else
		{
		    if (!ParseHost (&nHosts, &hostPtr, &nChars, 
				    hostOrAlias, len))
			goto sktoeol;
		    (*acPtr)->nhosts++;
		}
	    }
	    acPtr = &(*acPtr)->next;
	    nAcls++;
	}
    }

    if (error) {
	nHosts = nAliases = nAcls = nChars = 0;
      sendacl:
	LogError ("No XDMCP reqeusts will be granted\n");
    }
    GSendInt (nHosts);
    GSendInt (nAliases);
    GSendInt (nAcls);
    GSendInt (nChars);
    for (i = 0; i < nHosts; i++, hostList = hostList->next) {
	GSendInt (hostList->type);
	switch (hostList->type) {
	case HOST_ALIAS:
	    GSendStr (hostList->entry.aliasPattern);
	    break;
	case HOST_PATTERN:
	    GSendStr (hostList->entry.hostPattern);
	    break;
	case HOST_ADDRESS:
	    GSendArr (hostList->entry.displayAddress.hostAddrLen,
		      hostList->entry.displayAddress.hostAddress);
	    GSendInt (hostList->entry.displayAddress.connectionType);
	    break;
	}
    }
    for (i = 0; i < nAliases; i++, aliasList = aliasList->next) {
	GSendStr (aliasList->name);
	GSendInt (aliasList->hosts);
	GSendInt (aliasList->nhosts);
    }
    for (i = 0; i < nAcls; i++, acList = acList->next) {
	GSendInt (acList->entries);
	GSendInt (acList->nentries);
	GSendInt (acList->hosts);
	GSendInt (acList->nhosts);
	GSendInt (acList->flags);
    }
}


static struct displayMatch {
	const char	*name;
	int		len, type;
} displayTypes[] = {
	{ "local", 5,	dLocal | dPermanent | dFromFile },
	{ "foreign", 7,	dForeign | dPermanent | dFromFile },
};

#define def_type (Local | Permanent | FromFile)

static int
parseDisplayType (const char *string, const char **atPos)
{
    struct displayMatch	*d;

    *atPos = 0;
    for (d = displayTypes; d < displayTypes + as(displayTypes); d++) {
	if (!memcmp (d->name, string, d->len) &&
	    (!string[d->len] || string[d->len] == '@')) {
	    if (string[d->len] == '@')
		*atPos = string + d->len + 1;
	    return d->type;
	}
    }
    return -1;
}

typedef struct argV {
    struct argV	*next;
    const char	*str;
} ArgV;

typedef struct serverEntry {
    struct serverEntry	*next;
    const char	*name, *class2, *console;
    ArgV	*argv;
    int		type, argc;
} ServerEntry;

static void
ReadServersFile (const char *fname)
{
    const char	*word, *atPos;
    ServerEntry	*serverList, **serverPtr = &serverList;
    ArgV	*argv, **argp;
    File	file;
    int		i, j, len, nserv;

    nserv = 0;
    if (strcmp (fname, kdmrc)) {
	if (readFile (&file, fname, "X-Server specification"))
	    goto haveit;
    } else {
	ReadConf ();
	if (copyBuf (&file, VXservers.ptr, VXservers.len - 1))
	    goto haveit;
    }
    if (!copyBuf (&file, DEF_SERVER_LINE, sizeof (DEF_SERVER_LINE) - 1)) {
	LogInfo ("No X-Servers will be started\n");
	goto sendxs;
    }
  haveit:
    while ((word = ReadWord (&file, &len, FALSE))) {
Debug ("read display name %s\n", word);
	if (!(*serverPtr = (ServerEntry *) malloc (sizeof (ServerEntry)))) {
	    LogOutOfMem ("ReadServerFile");
	    break;
	}
	(*serverPtr)->name = word;
	/*
	 * extended syntax; if the second argument doesn't
	 * exactly match a legal display type and the third
	 * argument does, use the second argument as the
	 * display class string
	 */
	if (!(word = ReadWord (&file, &len, TRUE))) {
	  notype:
	    LogError ("Missing display type for %s in file %s\n", 
		      (*serverPtr)->name, fname);
	    continue;
	}
	(*serverPtr)->class2 = 0;
	(*serverPtr)->type = parseDisplayType (word, &atPos);
	if ((*serverPtr)->type < 0) {
	    (*serverPtr)->class2 = word;
Debug ("read display class %s\n", word);
	    if (!(word = ReadWord (&file, &len, TRUE)))
		goto notype;
	    (*serverPtr)->type = parseDisplayType (word, &atPos);
	    if ((*serverPtr)->type < 0) {
		while (ReadWord (&file, &len, TRUE));
		goto notype;
	    }
	}
Debug ("read display type %s\n", word);
	(*serverPtr)->console = atPos;
Debug ("read console %s\n", atPos);
	word = ReadWord (&file, &len, TRUE);
	if (word && !strcmp (word, "reserve")) {
Debug ("display is reserve\n");
	    (*serverPtr)->type = ((*serverPtr)->type & ~d_lifetime) | dReserve;
	    word = ReadWord (&file, &len, TRUE);
	}
	(*serverPtr)->argc = 0;
	argp = &(*serverPtr)->argv;
	while (word) {
Debug ("read server arg %s\n", word);
	    if (!(*argp = (ArgV *) malloc (sizeof (ArgV)))) {
		LogOutOfMem ("ReadServerFile");
		goto sendxs;
	    }
	    (*argp)->str = word;
	    argp = &(*argp)->next;
	    (*serverPtr)->argc++;
	    word = ReadWord (&file, &len, TRUE);
	}
	if (((*serverPtr)->type & d_location) == dLocal) {
	    if (!(*serverPtr)->argc) {
		LogError ("Missing arguments to local server %s in file %s\n",
			  (*serverPtr)->name, fname);
		continue;
	    }
	} else {
	    if ((*serverPtr)->argc) {
		LogError ("Superflous arguments to foreign server %s in file %s\n",
			  (*serverPtr)->name, fname);
		continue;
	    }
	}
	serverPtr = &(*serverPtr)->next;
	nserv++;
    }
  sendxs:
    GSendInt (nserv);
    for (i = 0; i < nserv; i++, serverList = serverList->next) {
	GSendStr (serverList->name);
	GSendStr (serverList->class2);
	GSendStr (serverList->console);
	GSendInt (serverList->type);
	j = serverList->argc;
	if (j) {
	    GSendInt (j + 1);
	    for (argv = serverList->argv; j; j--, argv = argv->next)
		GSendStr (argv->str);
	}
	GSendInt (0);
    }
}


#define T_S 0
#define T_N 1
#define T_Y 2
/*#define T_I 3*/

static struct {
	const char *name;
	int type;
	const char **dest;
} opts[] = {
	{ "-config", T_S, &kdmrc },
	{ "-daemon", T_Y, (const char **)&daemonize }, 
	{ "-nodaemon", T_N, (const char **)&daemonize },
	{ "-autolog", T_Y, (const char **)&autolog }, 
	{ "-noautolog", T_N, (const char **)&autolog },
};


#ifdef HAVE_PAM
Value pamservice = { KDM_PAM_SERVICE, sizeof(KDM_PAM_SERVICE) };
#endif

int main(int argc, char **argv)
{
    DSpec dspec;
    ValArr va;
    char *ci, *disp, *dcls, *cfgfile;
    int ap, what, i;

    if (!(ci = getenv("CONINFO"))) {
	fprintf(stderr, "This program is part of kdm and should not be run manually.\n");
	return 1;
    }
    if (sscanf (ci, "%d %d", &rfd, &wfd) != 2)
        return 1;

    InitLog();

    if ((debugLevel = GRecvInt ()) & DEBUG_WCONFIG)
	sleep (100);

/*Debug ("parsing command line\n");*/
    for (ap = 1; ap < argc; ap++) {
	for (i = 0; i < as(opts); i++)
	    if (!strcmp (argv[ap], opts[i].name)) {
		switch (opts[i].type) {
		case T_S:
		    if (ap + 1 == argc)
			goto misarg;
		    *opts[i].dest = argv[++ap];
		    break;
		case T_Y:
		    *(int *)opts[i].dest = 1;
		    break;
		case T_N:
		    *(int *)opts[i].dest = 0;
		    break;
/*		case T_I:
		    if (ap + 1 == argc || argv[ap + 1][0] == '-')
			goto misarg;
		    *(int *)opts[i].dest = atoi(argv[++ap]);
		    break;*/
		}
		goto oke;
	      misarg:
		LogError ("Missing argument to option %s\n", argv[ap]);
		goto oke;
	    }
	LogError ("Unknown command line option '%s'\n", argv[ap]);
      oke: ;
    }

    for (;;) {
/*	Debug ("Awaiting command ...\n");*/
	if (!GRecvCmd (&what))
	    break;
	switch (what) {
	case GC_Files:
/*	    Debug ("GC_Files\n");*/
	    ReadConf ();
	    CopyValues (&va, allSects + 0, 0, C_CONFIG);
	    CopyValues (&va, allSects + 1, 0, C_CONFIG);
	    GSendInt ((VXservers.ptr[0] == '/') ? 3 : 2);
	    GSendStr (kdmrc);
		GSendInt (-1);
	    GSendNStr (VXaccess.ptr, VXaccess.len - 1);
		GSendInt (0);
	    if (VXservers.ptr[0] == '/') {
		GSendNStr (VXservers.ptr, VXservers.len - 1);
		    GSendInt (0);
	    }
	    for (; (what = GRecvInt ()) != -1; )
		switch (what) {
		case GC_gGlobal:
		case GC_gDisplay:
		    GSendInt (0);
		    break;
		case GC_gXaccess:
		    GSendInt (1);
		    break;
		case GC_gXservers:
		    GSendInt ((VXservers.ptr[0] == '/') ? 2 : 0);
		    break;
		default:
		    GSendInt (-1);
		    break;
		}
	    break;
	case GC_GetConf:
/*	    Debug ("GC_GetConf\n");*/
	    memset (&va, 0, sizeof(va));
	    what = GRecvInt();
	    cfgfile = GRecvStr ();
	    switch (what) {
	    case GC_gGlobal:
/*		Debug ("GC_gGlobal\n");*/
		ReadConf ();
		CopyValues (&va, allSects + 0, 0, 0);
		CopyValues (&va, allSects + 1, 0, 0);
		CopyValues (&va, allSects + 2, 0, 0);
#ifdef HAVE_PAM
		AddValue (&va, C_PAMService, &pamservice);
#endif
		SendValues (&va);
		break;
	    case GC_gDisplay:
/*		Debug ("GC_gDisplay\n");*/
		disp = GRecvStr ();
/*		Debug (" Display %s\n", disp);*/
		dcls = GRecvStr ();
/*		Debug (" Class %s\n", dcls);*/
		MkDSpec (&dspec, disp, dcls ? dcls : "");
		ReadConf ();
		CopyValues (&va, allSects + 4, &dspec, 0);
		CopyValues (&va, allSects + 5, &dspec, 0);
		free (disp);
		if (dcls)
		    free (dcls);
		SendValues (&va);
		break;
	    case GC_gXservers:
		ReadServersFile (cfgfile);
		break;
	    case GC_gXaccess:
		ReadAccessFile (cfgfile);
		break;
	    default:
		Debug ("Unsupported config cathegory 0x%x\n", what);
	    }
	    free (cfgfile);
	    break;
	default:
	    Debug ("Unknown config command 0x%x\n", what);
	}
    }

/*    Debug ("Config reader exiting ...");*/
    return 0;
}
