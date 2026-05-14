// id_vh.c - Video Handlers (minimal SDL3 port)
// Wrappers around VL_* primitives. Provides the higher-level drawing
// API that the game logic uses (pics, bars, sprites, text).

#include "id_vh.h"
#include "id_vl.h"
#include "id_ca.h"
#include "id_mm.h"

#include <stdlib.h>
#include <string.h>

extern void Quit(char *error);

// -----------------------------------------------------------------------
// Public globals
// -----------------------------------------------------------------------

word FontColor = WHITE;
word BackColor = BLACK;
int  WindowX   = 0;
int  WindowY   = 0;
int  WindowW   = 320;
int  WindowH   = 200;
int  PrintX    = 0;
int  PrintY    = 0;

// Additional globals declared in headers
byte fontcolor = 0;
byte backcolor = 0;
int  fontnumber = 0;
byte gamepal[256 * 3] = {0};
pictabletype *pictable = NULL;

// ========================================================================
// Startup / Shutdown
// ========================================================================

void VW_Startup(void)
{
    VL_Startup();
}

void VW_Shutdown(void)
{
    VL_Shutdown();
}

// ========================================================================
// Text rendering
// ========================================================================

void VW_MeasurePropString(char *string, word *width, word *height)
{
    fontstruct *font = (fontstruct *)grsegs[STARTFONT + fontnumber];
    if (!font) { if (width) *width = 0; if (height) *height = 0; return; }

    if (height) *height = font->height;
    word w = 0;
    while (*string)
        w += font->width[(byte)*string++];
    if (width) *width = w;
}

void VW_DrawPropString(char *string)
{
    fontstruct *font = (fontstruct *)grsegs[STARTFONT + fontnumber];
    if (!font) return;

    int height = font->height;
    byte *fb = VL_GetFramebuffer();

    while (*string)
    {
        byte ch = (byte)*string++;
        int width = font->width[ch];
        if (width == 0) continue;

        byte *source = ((byte *)font) + font->location[ch];
        for (int col = 0; col < width; col++)
        {
            for (int row = 0; row < height; row++)
            {
                if (source[row * width + col])
                {
                    int sx = PrintX + col;
                    int sy = PrintY + row;
                    if (sx >= 0 && sx < VL_VGA_WIDTH && sy >= 0 && sy < VL_VGA_HEIGHT)
                        fb[bufferofs + sy * linewidth + sx] = fontcolor;
                }
            }
        }
        PrintX += width;
    }
}

// ========================================================================
// Tile drawing (stubs - will be filled in when tile system is ported)
// ========================================================================

void VW_DrawTile8(int x, int y, int tile)
{
    (void)x; (void)y; (void)tile;
}

void VW_DrawTile8M(int x, int y, int tile)
{
    (void)x; (void)y; (void)tile;
}

void VW_DrawTile16(int x, int y, int tile)
{
    (void)x; (void)y; (void)tile;
}

void VW_DrawTile16M(int x, int y, int tile)
{
    (void)x; (void)y; (void)tile;
}

void VW_DrawTile32(int x, int y, int tile)
{
    (void)x; (void)y; (void)tile;
}

void VW_DrawTile32M(int x, int y, int tile)
{
    (void)x; (void)y; (void)tile;
}

// ========================================================================
// Picture drawing
// ========================================================================

void VW_DrawPic(int x, int y, int picnum)
{
    CA_CacheGrChunk(picnum);

    if (!grsegs[picnum]) return;

    byte *data = (byte *)grsegs[picnum];
    if (!data) return;

    if (!pictable || picnum < STARTPICS || picnum >= STARTPICS + NUMPICS)
        return;

    int idx = picnum - STARTPICS;
    int pt_w = pictable[idx].width;
    int pt_h = pictable[idx].height;
    if (pt_w <= 0 || pt_h <= 0 || pt_w > 320 || pt_h > 200)
        return;
    if ((unsigned)pt_w * (unsigned)pt_h > grsegs_size[picnum])
        return;

    // Wolfenstein 3-D VGAGRAPH pics are stored as 4-plane planar VGA data:
    // the chunk is width*height bytes, ordered plane by plane, where pixel
    // column i belongs to plane (i & 3). De-plane it back to a linear block.
    // (Equivalent to Wolf4SDL's VL_MemToScreen.)
    byte *fb = VL_GetFramebuffer();
    byte *ptr = data;
    for (int plane = 0; plane < 4; plane++) {
        for (int row = 0; row < pt_h; row++) {
            int sy = y + row;
            for (int col = plane; col < pt_w; col += 4) {
                byte color = *ptr++;
                int sx = x + col;
                if (sx >= 0 && sx < VL_VGA_WIDTH && sy >= 0 && sy < VL_VGA_HEIGHT)
                    fb[bufferofs + sy * linewidth + sx] = color;
            }
        }
    }
}

// ========================================================================
// Drawing primitives (delegates to VL_*)
// ========================================================================

void VW_Bar(int x, int y, int width, int height, int color)
{
    VL_Bar((unsigned)x, (unsigned)y, (unsigned)width, (unsigned)height, (byte)color);
}

void VW_Plot(int x, int y, int color)
{
    VL_Plot((unsigned)x, (unsigned)y, (byte)color);
}

// Original Wolf3D convention: VW_Hlin(x1, x2, y) draws an inclusive
// horizontal span; VW_Vlin(y1, y2, x) draws an inclusive vertical span.
void VW_Hlin(int x1, int x2, int y, int color)
{
    VL_Hlin((unsigned)x1, (unsigned)y, (unsigned)(x2 - x1 + 1), (byte)color);
}

void VW_Vlin(int y1, int y2, int x, int color)
{
    VL_Vlin((unsigned)x, (unsigned)y1, (unsigned)(y2 - y1 + 1), (byte)color);
}

// ========================================================================
// Sprite drawing (stub)
// ========================================================================

void VW_DrawSprite(int x, int y, int sprite)
{
    (void)x; (void)y; (void)sprite;
    // Will be implemented when sprite system is ported
}

// ========================================================================
// Masked block blit (stub)
// ========================================================================

void VW_MaskBlock(byte *mask, int x, int y, int width, int height)
{
    (void)mask; (void)x; (void)y; (void)width; (void)height;
    // Will be implemented when masked blit is ported
}

// ========================================================================
// Screen update
// ========================================================================

void VW_UpdateScreen(void)
{
    VL_ScreenToScreen(bufferofs, displayofs, 320, 200);
    VL_Present();
}

// ========================================================================
// VWB_* double-buffer wrappers (in SDL3 port, just call VW_* directly)
// ========================================================================

void VWB_Bar(int x, int y, int w, int h, int color) { VW_Bar(x, y, w, h, color); }
void VWB_Plot(int x, int y, int color) { VW_Plot(x, y, color); }
void VWB_Hlin(int x1, int x2, int y, int color) { VW_Hlin(x1, x2, y, color); }
void VWB_Vlin(int y1, int y2, int x, int color) { VW_Vlin(y1, y2, x, color); }
void VWB_DrawPic(int x, int y, int picnum) { VW_DrawPic(x, y, picnum); }
void VWB_DrawPropString(char *str) { VW_DrawPropString(str); }
void VWB_DrawTile8(int x, int y, int tile) { VW_DrawTile8(x, y, tile); }
void VWB_DrawTile8M(int x, int y, int tile) { VW_DrawTile8M(x, y, tile); }
void VWB_DrawTile16(int x, int y, int tile) { VW_DrawTile16(x, y, tile); }
void VWB_DrawTile16M(int x, int y, int tile) { VW_DrawTile16M(x, y, tile); }
void VWB_DrawSprite(int x, int y, int sprite) { VW_DrawSprite(x, y, sprite); }
void VWB_UpdateScreen(void) { VW_UpdateScreen(); }

// ========================================================================
// Additional VW_* functions
// ========================================================================

void VW_FadeIn(void) { VL_FadeIn(0, 255, gamepal, 30); }
void VW_FadeOut(void) { VL_FadeOut(0, 255, 0, 0, 0, 30); }
void VW_WaitVBL(int vbls) { VL_WaitVBL(vbls); }
void VW_ScreenToScreen(unsigned src, unsigned dst, int w, int h) { VL_ScreenToScreen(src, dst, w, h); }

// ========================================================================
// Latch memory and fizzle fade
// ========================================================================

void LatchDrawPic(int x, int y, int picnum)
{
    VW_DrawPic(x, y, picnum);
}

void FizzleFade(unsigned src, unsigned dst, int width, int height, int steps, boolean abortable)
{
    // Simple fade - just copy and present
    VL_ScreenToScreen(src, dst, width, height);
    VL_Present();
    (void)steps; (void)abortable;
}

void LoadLatchMem(void)
{
    // Stub - latch memory not needed in SDL3 port
}

// ========================================================================
// Extension string for game data files
// ========================================================================

char extension[5] = "WL6";
