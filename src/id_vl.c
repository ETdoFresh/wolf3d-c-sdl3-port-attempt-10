// id_vl.c - Video Layer (SDL3 port)
// Replaces VGA hardware access (Mode 13h, planar VGA, port I/O)
// with SDL3 texture-based rendering.
//
// Architecture:
//   - 320x200 byte array as framebuffer (indexed 256-color)
//   - SDL3 window + renderer + streaming texture
//   - Palette stored as byte[256][3] RGB
//   - All drawing primitives write to the byte array
//   - VL_Present() copies framebuffer to texture and renders

#include "id_vl.h"

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// ------------------------------------------------------------------------
// Constants
// ------------------------------------------------------------------------

// VGA palette values are 6-bit (0-63). Scale to 8-bit (0-255).
#define VGA_TO_8BIT(v)  ((byte)((v) * 255 / 63))

#define FB_WIDTH    VL_VGA_WIDTH   // 320
#define FB_HEIGHT   VL_VGA_HEIGHT  // 200
#define FB_PAGES    3              // Triple-buffered VGA pages
#define FB_PAGESIZE (FB_WIDTH * FB_HEIGHT)
#define FB_SIZE     (FB_PAGESIZE * FB_PAGES)
#define PALETTE_SIZE 256
#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

// ------------------------------------------------------------------------
// Static SDL state
// ------------------------------------------------------------------------

static SDL_Window   *vl_window   = NULL;
static SDL_Renderer *vl_renderer = NULL;
static SDL_Texture  *vl_texture  = NULL;
static SDL_Palette  *vl_palette  = NULL;
static byte          vl_framebuffer[FB_SIZE];
static byte          vl_curpalette[PALETTE_SIZE][3];

// ------------------------------------------------------------------------
// Public globals (declared extern in id_vl.h)
// ------------------------------------------------------------------------

unsigned bufferofs  = 0;
unsigned displayofs = 0;
unsigned pelpan     = 0;
unsigned linewidth  = FB_WIDTH;  // 320 pixels per line
unsigned ylookup[MAXSCANLINES];
boolean  screenfaded = false;
unsigned bordercolor = 0;

// ========================================================================
// Internal helpers
// ========================================================================

// Build the ylookup table for the current linewidth
static void VL_BuildYLookup(void)
{
    for (int i = 0; i < MAXSCANLINES; i++)
        ylookup[i] = (unsigned)i * linewidth;
}

// Write a pixel at (x, y) in the active page (bufferofs)
static inline void VL_SetPixel(unsigned x, unsigned y, byte color)
{
    unsigned offset = bufferofs + ylookup[y] + x;
    if (offset >= sizeof(vl_framebuffer)) return;
    vl_framebuffer[offset] = color;
}

// Read a pixel at (x, y) in the active page
static inline byte VL_GetPixel(unsigned x, unsigned y)
{
    return vl_framebuffer[bufferofs + ylookup[y] + x];
}

byte *VL_GetFramebuffer(void)
{
    return vl_framebuffer;
}

void VL_PlotFast(unsigned offset, byte color)
{
    if (offset < sizeof(vl_framebuffer))
        vl_framebuffer[offset] = color;
}

// Palette is stored in vl_curpalette[][] and applied during VL_Present
static void VL_ApplyPalette(void)
{
    // No-op: palette is read directly during pixel conversion in VL_Present
}

// ========================================================================
// Init / Shutdown
// ========================================================================

void VL_Startup(void)
{
    // SDL_Init already called in main() — no need to re-init.
    // Just create window, renderer, and texture.

    vl_window = SDL_CreateWindow(
        "Wolfenstein 3D",
        WINDOW_WIDTH, WINDOW_HEIGHT,
        0
    );
    if (!vl_window) {
        fprintf(stderr, "VL_Startup: SDL_CreateWindow failed: %s\n", SDL_GetError());
        return;
    }

    vl_renderer = SDL_CreateRenderer(vl_window, NULL);
    if (!vl_renderer) {
        SDL_Log("VL_Startup: SDL_CreateRenderer failed: %s", SDL_GetError());
        return;
    }

    // Log which renderer we got
    SDL_PropertiesID props = SDL_GetRendererProperties(vl_renderer);
    const char *rname = SDL_GetStringProperty(props, SDL_PROP_RENDERER_NAME_STRING, "unknown");
    printf("VL_Startup: Using renderer: %s\n", rname);
    fflush(stdout);

    // Create the indexed-color palette
    vl_palette = SDL_CreatePalette(PALETTE_SIZE);
    if (!vl_palette) {
        SDL_Log("VL_Startup: SDL_CreatePalette failed: %s", SDL_GetError());
        return;
    }
    vl_texture = SDL_CreateTexture(
        vl_renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        FB_WIDTH, FB_HEIGHT
    );
    // Clear texture to black initially
    if (vl_texture) {
        Uint32 black[FB_WIDTH * FB_HEIGHT];
        memset(black, 0, sizeof(black));
        SDL_UpdateTexture(vl_texture, NULL, black, FB_WIDTH * sizeof(Uint32));
    }
    if (!vl_texture) {
        SDL_Log("VL_Startup: SDL_CreateTexture failed: %s", SDL_GetError());
        return;
    }

    // Nearest-neighbor scaling to preserve chunky pixel look
    SDL_SetTextureScaleMode(vl_texture, SDL_SCALEMODE_NEAREST);

    // Initialize state
    memset(vl_framebuffer, 0, sizeof(vl_framebuffer));
    memset(vl_curpalette, 0, sizeof(vl_curpalette));
    linewidth = FB_WIDTH;
    VL_BuildYLookup();
    bufferofs  = 0;
    displayofs = 0;
    pelpan     = 0;
    screenfaded = false;
    bordercolor = 0;
}

void VL_Shutdown(void)
{
    if (vl_texture) {
        SDL_DestroyTexture(vl_texture);
        vl_texture = NULL;
    }
    if (vl_palette) {
        SDL_DestroyPalette(vl_palette);
        vl_palette = NULL;
    }
    if (vl_renderer) {
        SDL_DestroyRenderer(vl_renderer);
        vl_renderer = NULL;
    }
    if (vl_window) {
        SDL_DestroyWindow(vl_window);
        vl_window = NULL;
    }
    SDL_Quit();
}

// ========================================================================
// Mode setting
// ========================================================================

void VL_SetVGAPlaneMode(void)
{
    // In the original this set VGA Mode 13h (320x200, 256 colors).
    // For SDL3 we just reset our framebuffer and ensure the window
    // is in a good state.
    memset(vl_framebuffer, 0, sizeof(vl_framebuffer));
    linewidth = FB_WIDTH;
    VL_BuildYLookup();
    bufferofs  = 0;
    displayofs = 0;
}

void VL_SetTextMode(void)
{
    // The original switched to 80x25 text mode.
    // We clear the framebuffer to black.
    memset(vl_framebuffer, 0, sizeof(vl_framebuffer));
    VL_FillPalette(0, 0, 0);
}

void VL_DePlaneVGA(void)
{
    // The original unchained VGA planar memory so all 256K was linear.
    // Nothing to do in SDL3; already linear.
}

void VL_ClearVideo(byte color)
{
    memset(vl_framebuffer, color, sizeof(vl_framebuffer));
}

// ========================================================================
// Display control
// ========================================================================

void VL_SetLineWidth(unsigned width)
{
    linewidth = width;
    VL_BuildYLookup();
}

void VL_SetSplitScreen(int linenum)
{
    // Original programmed the VGA CRTC split register.
    // Not needed for SDL3; no-op.
    (void)linenum;
}

void VL_WaitVBL(int vbls)
{
    // Original waited for vertical blank interrupts (70Hz VGA mode = ~14ms).
    for (int i = 0; i < vbls; i++)
        SDL_Delay(14);
}

void VL_CrtcStart(int crtc)
{
    // Original set the CRTC start address register for page flipping.
    // We just update displayofs.
    displayofs = (unsigned)crtc;
}

void VL_SetScreen(unsigned crtc, int pel)
{
    displayofs = crtc;
    pelpan = (unsigned)pel;
}

// ========================================================================
// Palette
// ========================================================================

void VL_FillPalette(byte red, byte green, byte blue)
{
    for (int i = 0; i < PALETTE_SIZE; i++) {
        vl_curpalette[i][0] = red;
        vl_curpalette[i][1] = green;
        vl_curpalette[i][2] = blue;
    }
    VL_ApplyPalette();
}

void VL_SetColor(int color, byte red, byte green, byte blue)
{
    if (color < 0 || color >= PALETTE_SIZE) return;
    vl_curpalette[color][0] = red;
    vl_curpalette[color][1] = green;
    vl_curpalette[color][2] = blue;
    VL_ApplyPalette();
}

void VL_GetColor(int color, byte *red, byte *green, byte *blue)
{
    if (color < 0 || color >= PALETTE_SIZE) return;
    *red   = vl_curpalette[color][0];
    *green = vl_curpalette[color][1];
    *blue  = vl_curpalette[color][2];
}

void VL_SetPalette(byte *palette)
{
    // palette is 768 bytes: 256 entries of [R, G, B]
    for (int i = 0; i < PALETTE_SIZE; i++) {
        vl_curpalette[i][0] = palette[i * 3 + 0];
        vl_curpalette[i][1] = palette[i * 3 + 1];
        vl_curpalette[i][2] = palette[i * 3 + 2];
    }
    VL_ApplyPalette();
}

void VL_GetPalette(byte *palette)
{
    for (int i = 0; i < PALETTE_SIZE; i++) {
        palette[i * 3 + 0] = vl_curpalette[i][0];
        palette[i * 3 + 1] = vl_curpalette[i][1];
        palette[i * 3 + 2] = vl_curpalette[i][2];
    }
}

void VL_SetPaletteImmediate(byte *palette)
{
    VL_SetPalette(palette);
}

void VL_FadeOut(int start, int end, int red, int green, int blue, int steps)
{
    byte origpal[768];
    VL_GetPalette(origpal);

    for (int step = 0; step <= steps; step++) {
        byte newpal[768];
        for (int i = start; i <= end; i++) {
            int base = i * 3;
            newpal[base + 0] = (byte)(origpal[base + 0] + (red   - origpal[base + 0]) * step / steps);
            newpal[base + 1] = (byte)(origpal[base + 1] + (green - origpal[base + 1]) * step / steps);
            newpal[base + 2] = (byte)(origpal[base + 2] + (blue  - origpal[base + 2]) * step / steps);
        }
        // Set entries outside [start,end] from origpal unchanged
        if (start > 0)
            memcpy(newpal, origpal, start * 3);
        if (end < PALETTE_SIZE - 1)
            memcpy(newpal + (end + 1) * 3, origpal + (end + 1) * 3, (PALETTE_SIZE - 1 - end) * 3);

        VL_SetPalette(newpal);
        VL_Present();
        VL_WaitVBL(1);
    }

    screenfaded = true;
}

void VL_FadeIn(int start, int end, byte *palette, int steps)
{
    byte curpal[768];
    VL_GetPalette(curpal);

    for (int step = 0; step <= steps; step++) {
        byte newpal[768];
        for (int i = start; i <= end; i++) {
            int base = i * 3;
            newpal[base + 0] = (byte)(curpal[base + 0] + (palette[base + 0] - curpal[base + 0]) * step / steps);
            newpal[base + 1] = (byte)(curpal[base + 1] + (palette[base + 1] - curpal[base + 1]) * step / steps);
            newpal[base + 2] = (byte)(curpal[base + 2] + (palette[base + 2] - curpal[base + 2]) * step / steps);
        }
        // Set entries outside [start,end] from target palette
        if (start > 0)
            memcpy(newpal, palette, start * 3);
        if (end < PALETTE_SIZE - 1)
            memcpy(newpal + (end + 1) * 3, palette + (end + 1) * 3, (PALETTE_SIZE - 1 - end) * 3);

        VL_SetPalette(newpal);
        VL_Present();
        VL_WaitVBL(1);
    }

    screenfaded = false;
}

void VL_ColorBorder(int color)
{
    bordercolor = (unsigned)color;
    // In SDL3 we do not draw a hardware border; the border color is
    // stored for compatibility. If desired, the renderer clear color
    // could be set to this palette entry.
}

void VL_TestPaletteSet(void)
{
    // No-op in SDL3; palette is always "hardware" accessible.
}

// ========================================================================
// Drawing primitives
// ========================================================================

void VL_Plot(unsigned x, unsigned y, byte color)
{
    VL_SetPixel(x, y, color);
}

void VL_Hlin(unsigned x, unsigned y, unsigned width, byte color)
{
    unsigned offset = bufferofs + ylookup[y] + x;
    if (offset + width > sizeof(vl_framebuffer)) return;
    memset(&vl_framebuffer[offset], color, width);
}

void VL_Vlin(unsigned x, unsigned y, unsigned height, byte color)
{
    unsigned offset = bufferofs + ylookup[y] + x;
    for (unsigned i = 0; i < height; i++) {
        if (offset >= sizeof(vl_framebuffer)) break;
        vl_framebuffer[offset] = color;
        offset += linewidth;
    }
}

void VL_Bar(unsigned x, unsigned y, unsigned width, unsigned height, byte color)
{
    unsigned offset = bufferofs + ylookup[y] + x;
    for (unsigned i = 0; i < height; i++) {
        if (offset + width > sizeof(vl_framebuffer)) break;
        memset(&vl_framebuffer[offset], color, width);
        offset += linewidth;
    }
}

// ========================================================================
// Image transfer
// ========================================================================

void VL_MungePic(byte *source, unsigned width, unsigned height)
{
    // The original VGA stored even scanlines first, then odd scanlines
    // (interlaced / planar layout). This function de-interlaces by
    // swapping every even/odd row pair back into sequential order.
    //
    // In our SDL3 port the data is already linear, but some original
    // assets may still be in interlaced order, so we perform the swap.
    byte *tmp = (byte *)malloc(width);
    if (!tmp) return;

    for (unsigned i = 0; i + 1 < height; i += 2) {
        byte *row_even = source + (unsigned)(i / 2) * width;               // compact even row
        byte *row_odd  = source + ((height / 2) + i / 2) * width;          // compact odd row
        // Swap: even row should be at i, odd row at i+1
        // Currently at sequential positions, but the original layout
        // puts even rows 0,2,4... first half and odd rows 1,3,5... second half.
        // After munging we want them in order 0,1,2,3...
        memcpy(tmp,      row_even, width);
        memcpy(row_even, row_odd,  width);
        memcpy(row_odd,  tmp,      width);
    }

    free(tmp);
}

void VL_MemToScreen(byte *source, int dest, int width, int height)
{
    // dest is a byte offset into the framebuffer (from the page start).
    // In the original VGA this was a segment:offset address.
    // We treat it as an offset within the flat framebuffer.
    unsigned x = (unsigned)(dest % (int)linewidth);
    unsigned y = (unsigned)(dest / (int)linewidth);

    for (int row = 0; row < height; row++) {
        if (y + row >= FB_HEIGHT) break;
        unsigned len = (unsigned)width;
        if (x + len > FB_WIDTH)
            len = FB_WIDTH - x;
        unsigned offset = bufferofs + ylookup[y + row] + x;
        memcpy(&vl_framebuffer[offset], source, len);
        source += width;
    }
}

void VL_ScreenToScreen(unsigned source, unsigned dest, int width, int height)
{
    // source and dest are byte offsets into the framebuffer.
    for (int row = 0; row < height; row++) {
        unsigned src_offset = source + (unsigned)row * linewidth;
        unsigned dst_offset = dest   + (unsigned)row * linewidth;
        if (src_offset + (unsigned)width > FB_SIZE) break;
        if (dst_offset + (unsigned)width > FB_SIZE) break;
        memmove(&vl_framebuffer[dst_offset], &vl_framebuffer[src_offset], (size_t)width);
    }
}

void VL_MemToLatch(byte *source, int width, int height, int dest)
{
    // The original copied source data into VGA latch memory at dest.
    // In our flat framebuffer model, latch memory is just another region
    // of the framebuffer. Copy directly.
    unsigned offset = (unsigned)dest;
    for (int row = 0; row < height; row++) {
        if (offset + (unsigned)width > FB_SIZE) break;
        memcpy(&vl_framebuffer[offset], source, (size_t)width);
        source += width;
        offset += linewidth;
    }
}

// Latch memory area starts after page 3
#define LATCH_START (FB_PAGESIZE * FB_PAGES)

// ========================================================================
// Masked blit (sprites with transparent color 0)
// ========================================================================

void VL_MaskedToScreen(byte *source, int dest, int width, int height)
{
    // source layout: first width*height bytes are the mask,
    // then width*height bytes are the pixel data.
    // Mask byte 0xFF = transparent (skip), otherwise draw pixel.
    // Alternatively, in many Wolf3D builds color index 0 is transparent.
    //
    // Standard Wolf3d convention: the first word of source is the width,
    // then height words of per-column offsets, then the actual data.
    // For simplicity we assume a flat source of width*height pixels
    // where color 0 is transparent.
    unsigned x = (unsigned)(dest % (int)linewidth);
    unsigned y = (unsigned)(dest / (int)linewidth);

    for (int row = 0; row < height; row++) {
        if (y + row >= FB_HEIGHT) break;
        for (int col = 0; col < width; col++) {
            byte pixel = source[row * width + col];
            if (pixel != 0) {
                VL_SetPixel(x + (unsigned)col, y + (unsigned)row, pixel);
            }
        }
    }
}

// Note: original Wolf3D had VL_DrawTile8String/VL_DrawLatch8String for 8x8
// tile-character text rendering, used only by debug routines. The port has
// no callers for either; declarations removed from id_vl.h.

void VL_SizeTile8String(char *str, int *width, int *height)
{
    if (width)  *width  = 0;
    if (height) *height = 8;
    (void)str;
}

void VL_DrawPropString(char *str, int basex)
{
    (void)str; (void)basex;
}

void VL_SizePropString(char *str, int *width, int *height)
{
    if (width)  *width  = 0;
    if (height) *height = 8;
    (void)str;
}

// ========================================================================
// Present / flip
// ========================================================================

// Pre-allocated conversion buffer (320*200 * 4 bytes = 256KB)
static Uint32 vl_convbuf[FB_WIDTH * FB_HEIGHT];

// Auto-screenshot for verification (saves at multiple frames)
// Only active when WOLF3D_SCREENSHOT environment variable is set
static int vl_frame_count = 0;
static int vl_screenshot_enabled = -1;  // -1 = not checked yet

void VL_Present(void)
{
    if (!vl_texture || !vl_renderer) return;

    // Cap at ~70fps
    static Uint64 last_present = 0;
    Uint64 now = SDL_GetTicks();
    if (last_present != 0 && (now - last_present) < 14) {
        SDL_Delay((Uint32)(14 - (now - last_present)));
    }
    last_present = SDL_GetTicks();

    // Convert indexed framebuffer to RGBA8888
    unsigned page_offset = displayofs;
    if (page_offset >= FB_SIZE) page_offset = 0;
    byte *src = &vl_framebuffer[page_offset];
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            byte idx = src[x];
            vl_convbuf[y * FB_WIDTH + x] =
                  ((Uint32)VGA_TO_8BIT(vl_curpalette[idx][0]) << 24)
                | ((Uint32)VGA_TO_8BIT(vl_curpalette[idx][1]) << 16)
                | ((Uint32)VGA_TO_8BIT(vl_curpalette[idx][2]) << 8)
                | 0xFFu;
        }
        src += linewidth;
    }

    // Upload to texture
    SDL_UpdateTexture(vl_texture, NULL, vl_convbuf, FB_WIDTH * sizeof(Uint32));

    // Draw stretched to fill window
    SDL_RenderClear(vl_renderer);
    SDL_RenderTexture(vl_renderer, vl_texture, NULL, NULL);
    SDL_RenderPresent(vl_renderer);

    // Auto-screenshot for self-verification (only with WOLF3D_SCREENSHOT env).
    // Dumps the indexed framebuffer straight to disk on a fixed frame
    // interval, so verification never has to fight for the OS foreground.
    vl_frame_count++;
    if (vl_screenshot_enabled < 0)
        vl_screenshot_enabled = SDL_getenv("WOLF3D_SCREENSHOT") ? 1 : 0;
    if (vl_screenshot_enabled) {
        static int shot_index = 0;
        if (vl_frame_count % 18 == 0 && shot_index < 400) {
            char path[64];
            snprintf(path, sizeof(path), "autoshot_%03d.bmp", shot_index++);
            VL_Screenshot(path);
            printf("Screenshot saved to %s\n", path);
            fflush(stdout);
        }
    }
}

int VL_Screenshot(const char *filename)
{
	unsigned page_offset = displayofs;
	if (page_offset >= FB_SIZE) page_offset = 0;
	byte *src = &vl_framebuffer[page_offset];

	SDL_Surface *surf = SDL_CreateSurface(320, 200, SDL_PIXELFORMAT_RGBA8888);
	if (!surf) return 0;
	Uint32 *px = (Uint32 *)surf->pixels;
	for (int y = 0; y < 200; y++) {
		for (int x = 0; x < 320; x++) {
			byte idx = src[y * linewidth + x];
			Uint32 color =
				  ((Uint32)VGA_TO_8BIT(vl_curpalette[idx][0]) << 24)
				| ((Uint32)VGA_TO_8BIT(vl_curpalette[idx][1]) << 16)
				| ((Uint32)VGA_TO_8BIT(vl_curpalette[idx][2]) << 8)
				| 0xFFu;
			px[y * (surf->pitch / 4) + x] = color;
		}
	}
	SDL_SaveBMP(surf, filename);
	SDL_DestroySurface(surf);
	return 1;
}
