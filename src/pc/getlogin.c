/* getlogin.c
 *
 * Author:   <rommel@ars.de>
 * Created: Wed Sep 30 1998
 */
 
static char *rcsid =
"$Id$";
static char *rcsrev = "$Revision$";

/*
 * $Log$ 
 */

#ifdef __MSDOS__

/* getlogin for DOS */

#ifdef __32BIT__

/* dummy code for 32-bit mode */

char *getlogin(void)
{
  return (char *) 0;
}

#else

/* real code for 16-bit mode */

#include <dos.h>
#include <string.h>

void doscall(union REGS *regs)
{
  struct SREGS sregs;
  segread(&sregs);
  sregs.es = sregs.ds;
  int86x(0x21, regs, regs, &sregs);
}

/* ----- Novell NetWare ----- */

#pragma pack(1)

/* Get Connection Information E3(16) */

struct _gcireq 
{
  unsigned short len;
  unsigned char func;
  unsigned char number;
};

struct _gcirep 
{
  unsigned short len;
  unsigned long objectID;
  unsigned short objecttype;
  char objectname[48];
  unsigned char logintime[7];
  unsigned char reserved[39];
};

#pragma pack()

char *nw_getlogin(void)
{
  static struct _gcirep gcirep;
  static struct _gcireq gcireq;

  union REGS r;

  /* Load Get Connection Number function code.   */
  r.x.ax = 0xDC00;
  doscall(&r);

  if (0 < r.h.al && r.h.al <= 100)
  {
    /* If the connection number is in range 1-100,
     * invoke Get Connection Information to get the user name. */

    gcireq.len = sizeof(gcireq) - sizeof(gcireq.len);
    gcireq.func = 0x16;
    gcireq.number = r.h.al;
    gcirep.len = sizeof(gcirep) - sizeof(gcirep.len);

    r.h.ah = 0xE3;
    r.x.si = (unsigned short) &gcireq;
    r.x.di = (unsigned short) &gcirep;
    doscall(&r);

    if (r.h.al == 0) 
    {
      strlwr(gcirep.objectname);
      return gcirep.objectname[0] ? gcirep.objectname : 0;
    }
  }

  return 0;
}

/* ----- Microsoft LAN Manager, Windows for Workgroups, IBM LAN Server ----- */

#pragma pack(1)

struct wksta_info_10
{
  char _far *computername;
  char _far *username;
  char _far *langroup;
  unsigned char ver_major;
  unsigned char ver_minor;
  char _far *logon_domain;
  char _far *oth_domains;
  char filler[32];
};

#pragma pack()

char *lan_getlogin(void)
{
  static struct wksta_info_10 wksta;
  static char name[32];
  union REGS r;

  r.x.ax = 0x5F44;
  r.x.bx = 10;
  r.x.cx = sizeof(wksta);
  r.x.di = (unsigned short) &wksta;
  doscall(&r);

  if (r.x.ax == 0 || r.x.ax == 0x5F44)
  {
    _fstrcpy(name, wksta.username);
    strlwr(name);
    return name[0] ? name : 0;
  }

  return 0;
}

#ifdef TEST

/* ----- testing code ----- */

void main(void)
{
  printf("NetWare: <%s>\nLAN Manager: <%s>\n",
	 nw_getlogin(), lan_getlogin());
}

#else

/* ----- main entry point ----- */

char *getlogin(void)
{
  char *lm = lan_getlogin();
  char *nw = nw_getlogin();
  return lm ? lm : nw ? nw : (char *) 0;
}

#endif

#endif /* __32BIT__ */

#endif /* __MSDOS__ */

#ifdef __OS2__

/* getlogin for OS/2 */

#define INCL_NOPM
#define INCL_DOS
#include <os2.h>
#include <string.h>

#pragma pack(1)

/* Because importing network API functions directly would prevent the
 * executables from loading on systems without the corresponding
 * network software, me must link them in during runtime manually with
 * DosLoadModule/DosGetProcAddress/DosFreeModule. This gets even more
 * difficult and ugly in 32-bit mode, since we have to call the same
 * old 16-bit API's and thunking from some compilers is a bit difficult. 
 */

#ifdef __386__
#ifndef __32BIT__
#define __32BIT__ 1
#endif
#endif

#ifndef __32BIT__
#define DosQueryProcAddr(handle, ord, name, funcptr) \
	DosGetProcAddr(handle, name, funcptr)
#endif

/* ----- Novell NetWare ----- */

struct info 
{
  USHORT connectionID;
  USHORT connectFlags;
  USHORT sessionID;
  USHORT connectionNumber;
  CHAR serverAddr[12];
  USHORT serverType;
  CHAR serverName[48];
  USHORT clientType;
  CHAR clientName[48];
};

typedef struct info FAR *PINFO;

#ifdef __EMX__

USHORT (APIENTRY *NWGDCID16)(PUSHORT pID);
USHORT (APIENTRY *NWGCS16)(USHORT nID, PINFO pInfo, USHORT nSize);

USHORT NWGetDefaultConnectionID(PUSHORT pID)
{
  return (USHORT)
          (_THUNK_PROLOG (4);
           _THUNK_FLAT (pID);
           _THUNK_CALLI (_emx_32to16(NWGDCID16)));
}

USHORT NWGetConnectionStatus(USHORT nID, PINFO pInfo, USHORT nSize)
{
  return (USHORT)
          (_THUNK_PROLOG (2+4+2);
           _THUNK_SHORT (nID);
           _THUNK_FLAT (pInfo);
           _THUNK_SHORT (nSize);
           _THUNK_CALLI (_emx_32to16(NWGCS16)));
}

#else

#ifdef __32BIT__

USHORT (* APIENTRY16 NWGetDefaultConnectionID)(PVOID16 pID);
USHORT (* APIENTRY16 NWGetConnectionStatus)(USHORT nID, PVOID16 pInfo, USHORT nSize);

#else

USHORT (APIENTRY * NWGetDefaultConnectionID)(PUSHORT pID);
USHORT (APIENTRY * NWGetConnectionStatus)(USHORT nID, PINFO pInfo, USHORT nSize);

#endif

#define NWGDCID16 NWGetDefaultConnectionID
#define NWGCS16 NWGetConnectionStatus

#endif

char *nw_getlogin(void)
{
  static struct info ci;

  HMODULE NWCalls;
  USHORT id;
  char buf[256], *res = NULL;

  if (DosLoadModule(buf, sizeof(buf), "NWCALLS", &NWCalls) == 0) 
  {
    if (DosQueryProcAddr(NWCalls, 0, "NWGETDEFAULTCONNECTIONID", 
			 (PVOID) &NWGDCID16) == 0 &&
        DosQueryProcAddr(NWCalls, 0, "NWGETCONNECTIONSTATUS", 
			 (PVOID) &NWGCS16) == 0)
    {
#if defined(__WATCOMC__) && defined(__386__)
      NWGDCID16 = (PVOID) (ULONG) (PVOID16) NWGDCID16;
      NWGCS16 = (PVOID) (ULONG) (PVOID16) NWGCS16;
#endif

      if (NWGetDefaultConnectionID(&id) == 0 && 
	  NWGetConnectionStatus(id, &ci, sizeof(ci)) == 0)
      {
	strlwr(ci.clientName);
	if (ci.clientName[0])
	  res = ci.clientName;
      }
    }

    DosFreeModule(NWCalls);
  }

  return res;
}

/* ----- Microsoft LAN Manager, IBM LAN Server ----- */

#ifdef __EMX__

#define PSTR _far16ptr
#define _fstrcpy(d, s) strcpy(d, _emx_16to32(s))

USHORT (APIENTRY *NWGI16)(PSTR pszServer, USHORT sLevel, 
  PVOID pbBuffer, USHORT cbBuffer, PUSHORT pcbTotalAvail);

USHORT NetWkstaGetInfo(PSZ pszServer, USHORT sLevel, PVOID pbBuffer, 
		       USHORT cbBuffer, PUSHORT pcbTotalAvail)
{
  return (USHORT)
          (_THUNK_PROLOG (4+2+4+2+4);
           _THUNK_FLAT (pszServer);
           _THUNK_SHORT (sLevel);
           _THUNK_FLAT (pbBuffer);
           _THUNK_SHORT (cbBuffer);
           _THUNK_FLAT (pcbTotalAvail);
           _THUNK_CALLI (_emx_32to16(NWGI16)));
}

#else

#ifdef __32BIT__

#define PSTR PCHAR16

#ifdef __WATCOMC__
#define _fstrcpy(d, s) {char *t = s; strcpy(d, t);}
#else
#define _fstrcpy(d, s) strcpy(d, s)
#endif

USHORT (* APIENTRY16 NetWkstaGetInfo)(PSTR pszServer, USHORT sLevel, 
  PVOID16 pbBuffer, USHORT cbBuffer, PVOID16 pcbTotalAvail);

#else

#define PSTR PSZ

USHORT (APIENTRY *NetWkstaGetInfo)(PSTR pszServer, USHORT sLevel, 
  PVOID pbBuffer, USHORT cbBuffer, PUSHORT pcbTotalAvail);

#endif

#define NWGI16 NetWkstaGetInfo

#endif

struct wksta_info_10
{
  PSTR computername;
  PSTR username;
  PSTR langroup;
  BYTE ver_major;
  BYTE ver_minor;
  PSTR logon_domain;
  PSTR oth_domains;
  BYTE filler[32];
};

char *lan_getlogin(void)
{
  struct wksta_info_10 wksta;
  static char name[32];

  HMODULE NETAPI;
  USHORT total;
  char buf[256], *res = NULL;

  if (DosLoadModule(buf, sizeof(buf), "NETAPI", &NETAPI) == 0) 
  {
    if (DosQueryProcAddr(NETAPI, 0, "NETWKSTAGETINFO", (PVOID) &NWGI16) == 0)
    {
#if defined(__WATCOMC__) && defined(__386__)
      NWGI16 = (PVOID) (ULONG) (PVOID16) NWGI16;
#endif

      if (NetWkstaGetInfo(0, 10, &wksta, sizeof(wksta), &total) == 0)
      {
	_fstrcpy(name, wksta.username);
	strlwr(name);
	if (name[0])
	  res = name;
      }
    }

    DosFreeModule(NETAPI);
  }

  return res;
}

#ifdef TEST

/* ----- testing code ----- */

void main(void)
{
  printf("NetWare: <%s>\nLAN Manager: <%s>\n", 
	 nw_getlogin(), lan_getlogin());
}

#else

/* ----- main entry point ----- */

char *getlogin(void)
{
  char *lm = lan_getlogin();
  char *nw = nw_getlogin();
  return lm ? lm : nw ? nw : (char *) 0;
}

#endif

#endif /* __OS2__ */

#if defined(_WIN32) || defined(__WIN32__)

/* getlogin for NT/Win95 */

#if defined(__WATCOMC__) || defined(_MSC_VER) || defined(__IBMC__) || defined(__MINGW32__) || defined(__CYGWIN__)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
extern GetUserNameA(char *buffer, unsigned long *size);
#define GetUserName GetUserNameA
#endif

char *getlogin(void)
{
#ifndef DEBUG
  static char buffer[256];
  unsigned long size = sizeof(buffer);

  if (GetUserName(buffer, &size))
    return buffer[0] ? buffer : 0;
#endif

  return 0;
}

#ifdef TEST

void main(void)
{
  printf("Win32: <%s>\n", getlogin());
}

#endif

#endif /* __WIN32__ */

/* end of getlogin.c */
