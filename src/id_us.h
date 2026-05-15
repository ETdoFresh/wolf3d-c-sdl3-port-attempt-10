// id_us.h - User Services header (SDL3 port)
#ifndef ID_US_H
#define ID_US_H

#include "wl_types.h"

#define MaxX         320
#define MaxY         200
#define MaxHelpLines 500
#define MaxHighName  57
#define MaxScores    7
#define MaxGameName  32
#define MaxSaveGames 6
#define MaxString    128

#define US_HomeWindow() { PrintX = WindowX; PrintY = WindowY; }

typedef struct {
    char     name[58];
    long     score;
    word     completed, episode;
} HighScore;

typedef struct {
    char     signature[4];
    word     *oldtest;
    boolean  present;
    char     name[33];
} SaveGame;

typedef enum { gd_Continue, gd_Easy, gd_Normal, gd_Hard } GameDiff;

typedef struct {
    int x, y, w, h, PrintX, PrintY;
} WindowRec;

extern boolean ingame, abortgame, loadedgame, NoWait;
extern boolean HighScoresDirty, abortprogram, restartgame;
extern int     PrintX, PrintY, WindowX, WindowY, WindowW, WindowH;
extern boolean Button0, Button1, CursorBad;
extern int     CursorX, CursorY;

// Function pointers for string measurement/drawing
extern void (*USL_MeasureString)(const char *, word *, word *);
extern void (*USL_DrawString)(const char *);
extern SaveGame Games[MaxSaveGames];
extern HighScore Scores[MaxScores];
extern int    tedlevel, tedlevelnum;

void US_Startup(void);
void US_Setup(void);
void US_Shutdown(void);
void US_InitRndT(boolean randomize);
int  US_RndT(void);
void US_SetLoadSaveHooks(boolean (*save)(int), boolean (*load)(int), void (*reset)(void));
void US_TextScreen(void);
void US_UpdateTextScreen(void);
void US_FinishTextScreen(void);
void US_DrawWindow(int x, int y, int w, int h);
void US_CenterWindow(int w, int h);
void US_SaveWindow(WindowRec *win);
void US_RestoreWindow(WindowRec *win);
void US_ClearWindow(void);
void US_SetPrintRoutines(void (*measure)(const char *, word *, word *), void (*draw)(const char *));
void US_PrintCentered(const char *s);
void US_CPrint(const char *s);
void US_CPrintLine(const char *s);
void US_Print(const char *s);
void US_PrintUnsigned(longword n);
void US_PrintSigned(long n);
void US_StartCursor(void);
void US_ShutCursor(void);
boolean US_UpdateCursor(void);
boolean US_LineInput(int x, int y, char *buf, char *def, boolean escok, int maxchars, int maxwidth);
int  US_CheckParm(char *parm, char **strings);
boolean US_UserInput(longword ticks);
void US_DisplaySaving(void);
void TEDDeath(void);

#endif
