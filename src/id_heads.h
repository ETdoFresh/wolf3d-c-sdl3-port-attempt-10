// id_heads.h - Master include header (SDL3 port)
// Replaces the original ID_HEADS.H with modern C equivalents.
#ifndef ID_HEADS_H
#define ID_HEADS_H

// Modern C headers replacing DOS headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "wl_config.h"
#include "wl_types.h"

// Asset definition headers
#include "gfxv_wl6.h"
#include "audiowl6.h"
#include "mapswl6.h"

// Engine headers
#include "id_mm.h"
#include "id_pm.h"
#include "id_ca.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_in.h"
#include "id_sd.h"
#include "id_us.h"

#define GREXT "VGA"

#define PORTTILESWIDE    20
#define PORTTILESHIGH    13
#define UPDATEWIDE       PORTTILESWIDE
#define UPDATEHIGH       PORTTILESHIGH
#define MAXTICS          10
#define DEMOTICS         4
#define UPDATETERMINATE  0x0301

extern unsigned mapwidth, mapheight, tics;
extern boolean  compatability;

extern byte     *updateptr;
extern unsigned uwidthtable[UPDATEHIGH];
extern unsigned blockstarts[UPDATEWIDE * UPDATEHIGH];

extern byte     fontcolor, backcolor;
extern int      fontnumber;

#define SETFONTCOLOR(f, b) { fontcolor = (f); backcolor = (b); }

void Quit(char *error);

// File extension for game data
extern char extension[5];

#endif
