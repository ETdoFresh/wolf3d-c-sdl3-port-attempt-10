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

static unsigned long rndval = 1;   // LCG seed

// ========================================================================
// Startup / Shutdown
// ========================================================================

void US_Startup(void)
{
    // Initialize random seed
    rndval = (unsigned long)time(NULL);

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
    if (randomize) {
        rndval = (unsigned long)time(NULL);
    } else {
        rndval = 1;
    }
}

int US_RndT(void)
{
    // Linear Congruential Generator
    // Using the same parameters as the original Borland LCG:
    //   next = prev * 1103515245 + 12345
    // Returns a value in 0..255 range (low byte of shifted result)
    rndval = rndval * 1103515245UL + 12345UL;
    return (int)((rndval >> 16) & 0xFF);
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

// ========================================================================
// High scores (stubs)
// ========================================================================

void US_CheckHighScore(long score, int other)
{
    (void)score;
    (void)other;
}

void US_DisplayHighScores(int which)
{
    (void)which;
}

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
