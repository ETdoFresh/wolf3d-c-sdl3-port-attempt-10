// id_us_1.c - User Services (minimal SDL3 port)
// Provides basic user interface functions: window drawing, text output,
// random numbers, command-line parsing, high scores.
// Most functions are stubs initially.

#include "id_us.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_in.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

extern void Quit(char *error);

// -----------------------------------------------------------------------
// Public globals
// -----------------------------------------------------------------------

boolean  abortgame      = false;
boolean  loadedgame     = false;
boolean  NoWait         = false;
boolean  HighScoresDirty = false;
boolean  abortprogram   = false;
boolean  restartgame    = false;
boolean  Button0        = false;
boolean  Button1        = false;
boolean  CursorBad      = false;

// Forward declarations from id_vh.c
extern void VW_MeasurePropString(char *string, word *width, word *height);
extern void VW_DrawPropString(char *string);

// Function pointers for string measurement/drawing
void (*USL_MeasureString)(char *, word *, word *) = VW_MeasurePropString;
void (*USL_DrawString)(char *) = VW_DrawPropString;
int      CursorX        = 0;
int      CursorY        = 0;
SaveGame Games[MaxSaveGames];
HighScore Scores[MaxScores];

// -----------------------------------------------------------------------
// Internal state
// -----------------------------------------------------------------------

// Original Wolfenstein 3-D random number table.  US_RndT walks this fixed
// 256-byte table by an index; recorded demos depend on the exact sequence,
// so this must match the original byte-for-byte (an LCG desyncs demos).
static const byte rndtable[256] = {
      0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66,
     74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36,
     95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188,
     52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235,
     25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113,
     94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113,
     80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241,
     24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95,
     28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226,
     71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36,
     17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136,
    120, 163, 236, 249
};
static int rndindex = 0;

// ========================================================================
// Startup / Shutdown
// ========================================================================

void US_Startup(void)
{
    // Initialize random table index
    US_InitRndT(true);

    // Clear high scores
    memset(Scores, 0, sizeof(Scores));

    // Clear save game slots
    memset(Games, 0, sizeof(Games));

    // Default window
    WindowX = 0;
    WindowY = 0;
    WindowW = MaxX;
    WindowH = MaxY;
    PrintX  = WindowX;
    PrintY  = WindowY;

    ingame         = false;
    abortgame      = false;
    loadedgame     = false;
    NoWait         = false;
    HighScoresDirty = false;
    abortprogram   = false;
    restartgame    = false;
    tedlevel       = 0;
    tedlevelnum    = 0;
}

void US_Setup(void)
{
    // No-op: additional setup if needed
}

void US_Shutdown(void)
{
    // No-op
}

// ========================================================================
// Random number generator (LCG)
// ========================================================================

void US_InitRndT(boolean randomize)
{
    // randomize: seed the index from the clock (normal play).
    // !randomize: index 0, the deterministic state demos were recorded with.
    if (randomize)
        rndindex = (int)(time(NULL) & 0xFF);
    else
        rndindex = 0;
}

int US_RndT(void)
{
    rndindex = (rndindex + 1) & 0xFF;
    return rndtable[rndindex];
}

// ========================================================================
// Load/Save hooks (stubs)
// ========================================================================

static boolean (*save_hook)(int)   = NULL;
static boolean (*load_hook)(int)   = NULL;
static void    (*reset_hook)(void) = NULL;

void US_SetLoadSaveHooks(boolean (*save)(int), boolean (*load)(int), void (*reset)(void))
{
    save_hook  = save;
    load_hook  = load;
    reset_hook = reset;
}

// ========================================================================
// Text screen (stubs)
// ========================================================================

void US_TextScreen(void)
{
    // No-op in SDL3 port
}

void US_UpdateTextScreen(void)
{
    // No-op
}

void US_FinishTextScreen(void)
{
    // No-op
}

// ========================================================================
// Window drawing
// ========================================================================

void US_DrawWindow(int x, int y, int w, int h)
{
    WindowX = x;
    WindowY = y;
    WindowW = w;
    WindowH = h;
    PrintX  = x;
    PrintY  = y;

    // Draw a bordered rectangle
    // Outer border (dark)
    VW_Bar(x - 1, y - 1, w + 2, h + 2, DARKGRAY);
    // Inner fill (black)
    VW_Bar(x, y, w, h, BLACK);
}

void US_CenterWindow(int w, int h)
{
    int x = (MaxX - w) / 2;
    int y = (MaxY - h) / 2;
    US_DrawWindow(x, y, w, h);
}

void US_SaveWindow(WindowRec *win)
{
    if (!win) return;
    win->x  = WindowX;
    win->y  = WindowY;
    win->w  = WindowW;
    win->h  = WindowH;
    win->px = PrintX;
    win->py = PrintY;
}

void US_RestoreWindow(WindowRec *win)
{
    if (!win) return;
    WindowX = win->x;
    WindowY = win->y;
    WindowW = win->w;
    WindowH = win->h;
    PrintX  = win->px;
    PrintY  = win->py;
}

void US_ClearWindow(void)
{
    VW_Bar(WindowX, WindowY, WindowW, WindowH, BLACK);
    PrintX = WindowX;
    PrintY = WindowY;
}

// ========================================================================
// Print routines (stubs)
// ========================================================================

static void (*custom_measure)(char *, word *, word *) = NULL;
static void (*custom_draw)(char *) = NULL;

void US_SetPrintRoutines(void (*measure)(char *, word *, word *), void (*draw)(char *))
{
    custom_measure = measure;
    custom_draw    = draw;
}

void US_PrintCentered(char *s)
{
    word w, h;
    USL_MeasureString(s, &w, &h);
    PrintX = WindowX + ((WindowW - w) / 2);
    US_Print(s);
}

void US_CPrint(char *s)
{
    char c, *se;
    word w, h;

    while (*s)
    {
        se = s;
        while ((c = *se) && (c != '\n'))
            se++;

        char linebuf[256];
        int len = (int)(se - s);
        if (len >= (int)sizeof(linebuf)) len = (int)sizeof(linebuf) - 1;
        memcpy(linebuf, s, len);
        linebuf[len] = '\0';

        USL_MeasureString(linebuf, &w, &h);
        PrintX = WindowX + ((WindowW - w) / 2);
        USL_DrawString(linebuf);

        s = se;
        if (c)
        {
            s++;
            PrintY += h;
        }
    }
}

void US_CPrintLine(char *s)
{
    US_CPrint(s);
}

void US_Print(char *s)
{
    char c, *se;
    word w, h;
    char linebuf[256];

    while (*s)
    {
        se = s;
        while ((c = *se) && (c != '\n'))
            se++;

        int len = (int)(se - s);
        if (len >= (int)sizeof(linebuf)) len = (int)sizeof(linebuf) - 1;
        memcpy(linebuf, s, len);
        linebuf[len] = '\0';

        USL_MeasureString(linebuf, &w, &h);
        USL_DrawString(linebuf);

        s = se;
        if (c)
        {
            s++;
            PrintX = WindowX;
            PrintY += h;
        }
        else
        {
            PrintX += w;
        }
    }
}

void US_PrintUnsigned(longword n)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)n);
    US_Print(buf);
}

void US_PrintSigned(long n)
{
    char buf[16];
    snprintf(buf, sizeof(buf), "%ld", n);
    US_Print(buf);
}

// ========================================================================
// Cursor (stubs)
// ========================================================================

void US_StartCursor(void)
{
    CursorBad = false;
}

void US_ShutCursor(void)
{
    // No-op
}

boolean US_UpdateCursor(void)
{
    return false;
}

// ========================================================================
// Line input (stub)
// ========================================================================

boolean US_LineInput(int x, int y, char *buf, char *def, boolean escok,
                     int maxchars, int maxwidth)
{
    // Stub: return empty string immediately
    if (buf) buf[0] = '\0';
    (void)x; (void)y; (void)def; (void)escok;
    (void)maxchars; (void)maxwidth;
    return true;
}

// ========================================================================
// Command-line parameter parsing
// ========================================================================

int US_CheckParm(char *parm, char **strings)
{
    if (!parm || !strings) return -1;

    // Try to match parm against each string in the array.
    // Check both with and without leading dash/slash.
    int i;
    for (i = 0; strings[i] != NULL; i++) {
        // Direct match
        if (_stricmp(parm, strings[i]) == 0) {
            return i;
        }
        // Match with leading '-' or '/'
        if (parm[0] == '-' || parm[0] == '/') {
            if (_stricmp(parm + 1, strings[i]) == 0) {
                return i;
            }
        }
    }

    return -1;
}

// ========================================================================
// User input (timed wait)
// ========================================================================

boolean US_UserInput(longword ticks)
{
    return IN_UserInput(ticks);
}

// Note: Wolf3D's actual high-score handling is implemented as CheckHighScore
// and DrawHighScores in wl_inter.c (matching the original WL_INTER.C). The
// US_*HighScore declarations in id_us.h are vestigial from id_us.h's pattern
// and not part of the active code path.

void US_DisplaySaving(void)
{
    // No-op
}

// ========================================================================
// TED death (editor support)
// ========================================================================

void TEDDeath(void)
{
    Quit("TEDDeath: Unexpected TED editor call");
}
