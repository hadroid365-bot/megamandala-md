#include <genesis.h>

/*
    MEGAMANDALA MD
    v0.4 - BOTOP TA DE OLHO
    SGDK / Marsdev safe build

    Regras:
    - Sem asset externo.
    - Sem TILE_USERINDEX antigo.
    - Sem float.
    - Sem malloc.
    - Sem outro main.c em src/.
*/

#define SCREEN_TILES_X      40
#define SCREEN_TILES_Y      28
#define MAX_LINES           240

#define TILE_BASE           TILE_USER_INDEX

#define TILE_EMPTY          (TILE_BASE + 0)
#define TILE_CHECKER        (TILE_BASE + 1)
#define TILE_LINES          (TILE_BASE + 2)
#define TILE_DOT            (TILE_BASE + 3)
#define TILE_BAR_V          (TILE_BASE + 4)
#define TILE_BAR_H          (TILE_BASE + 5)
#define TILE_BRACKET        (TILE_BASE + 6)
#define TILE_DIAG_A         (TILE_BASE + 7)
#define TILE_DIAG_B         (TILE_BASE + 8)
#define TILE_CROSS          (TILE_BASE + 9)
#define TILE_NOISE          (TILE_BASE + 10)
#define TILE_SCAN           (TILE_BASE + 11)

#define TILE_COUNT          12

#define SCENE_MACRONIC      0
#define SCENE_MANDALA       1
#define SCENE_TUNNEL        2
#define SCENE_GLITCH        3
#define SCENE_BREATH        4
#define SCENE_EYE           5
#define SCENE_COUNT         6

static u16 frameCounter = 0;
static u16 sceneTimer = 0;
static u16 currentScene = SCENE_MACRONIC;
static u16 previousPad = 0;
static u16 seed = 0xB00Du;

static s16 hscroll[MAX_LINES];

static u16 pal1Work[16];
static u16 pal2Work[16];
static u16 pal3Work[16];

/* ------------------------------------------------------------ */
/* Tiles 8x8, 4bpp. Cada u32 representa uma linha de 8 pixels.  */
/* Cada nibble é índice de cor da paleta escolhida pelo tilemap. */
/* ------------------------------------------------------------ */

static const u32 tile_empty[8] =
{
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static const u32 tile_checker[8] =
{
    0x10101010, 0x01010101, 0x10101010, 0x01010101,
    0x10101010, 0x01010101, 0x10101010, 0x01010101
};

static const u32 tile_lines[8] =
{
    0x11111111, 0x00000000, 0x22222222, 0x00000000,
    0x11111111, 0x00000000, 0x22222222, 0x00000000
};

static const u32 tile_dot[8] =
{
    0x00000000, 0x00033000, 0x00333300, 0x03333330,
    0x03333330, 0x00333300, 0x00033000, 0x00000000
};

static const u32 tile_bar_v[8] =
{
    0x00033000, 0x00033000, 0x00033000, 0x00033000,
    0x00033000, 0x00033000, 0x00033000, 0x00033000
};

static const u32 tile_bar_h[8] =
{
    0x00000000, 0x00000000, 0x33333333, 0x33333333,
    0x00000000, 0x00000000, 0x11111111, 0x00000000
};

static const u32 tile_bracket[8] =
{
    0x03333300, 0x03000000, 0x03000000, 0x03330000,
    0x03330000, 0x03000000, 0x03000000, 0x03333300
};

static const u32 tile_diag_a[8] =
{
    0x30000000, 0x03000000, 0x00300000, 0x00030000,
    0x00003000, 0x00000300, 0x00000030, 0x00000003
};

static const u32 tile_diag_b[8] =
{
    0x00000003, 0x00000030, 0x00000300, 0x00003000,
    0x00030000, 0x00300000, 0x03000000, 0x30000000
};

static const u32 tile_cross[8] =
{
    0x30000003, 0x03000030, 0x00300300, 0x00033000,
    0x00033000, 0x00300300, 0x03000030, 0x30000003
};

static const u32 tile_noise[8] =
{
    0x10301030, 0x01020102, 0x30103010, 0x02010201,
    0x10301030, 0x01020102, 0x30103010, 0x02010201
};

static const u32 tile_scan[8] =
{
    0x11111111, 0x00000000, 0x00000000, 0x22222222,
    0x00000000, 0x00000000, 0x11111111, 0x00000000
};

/* ------------------------------------------------------------ */
/* Paletas Mega Drive: formato SGDK/VDP 0000 BBB0 GGG0 RRR0     */
/* ------------------------------------------------------------ */

static const u16 pal0Text[16] =
{
    0x0000, 0x0222, 0x0444, 0x0666,
    0x0888, 0x0AAA, 0x0CCC, 0x0EEE,
    0x0000, 0x000E, 0x00AE, 0x0EEE,
    0x0AAE, 0x0CCE, 0x0EEE, 0x0EEE
};

static const u16 pal1Hot[16] =
{
    0x0000, 0x0022, 0x0044, 0x0066,
    0x0088, 0x00AA, 0x00CC, 0x00EE,
    0x022E, 0x044E, 0x066E, 0x088E,
    0x0AAE, 0x0CCE, 0x0EEE, 0x0AAA
};

static const u16 pal2Cold[16] =
{
    0x0000, 0x0202, 0x0404, 0x0606,
    0x0808, 0x0A0A, 0x0C0C, 0x0E0E,
    0x0E00, 0x0E20, 0x0E40, 0x0E60,
    0x0E80, 0x0EA0, 0x0EC0, 0x0EEE
};

static const u16 pal3Ghost[16] =
{
    0x0000, 0x0220, 0x0440, 0x0660,
    0x0880, 0x0AA0, 0x0CC0, 0x0EE0,
    0x0222, 0x0444, 0x0666, 0x0888,
    0x0AAA, 0x0CCC, 0x0EEE, 0x0ACE
};

/* ------------------------------------------------------------ */

static u16 rng16(void)
{
    seed = (u16)(seed * 109u + 89u);
    return seed;
}

static void copyPalette(const u16 *src, u16 *dst)
{
    u16 i;

    for (i = 0; i < 16; i++)
        dst[i] = src[i];
}

static u16 makeColor(u16 r, u16 g, u16 b)
{
    return (u16)((b << 9) | (g << 5) | (r << 1));
}

static u16 brightenColor(u16 color, u16 amount)
{
    u16 r = (color >> 1) & 7;
    u16 g = (color >> 5) & 7;
    u16 b = (color >> 9) & 7;

    r = (r + amount > 7) ? 7 : r + amount;
    g = (g + amount > 7) ? 7 : g + amount;
    b = (b + amount > 7) ? 7 : b + amount;

    return makeColor(r, g, b);
}

static void loadTiles(void)
{
    VDP_loadTileData(tile_empty,   TILE_EMPTY,   1, DMA);
    VDP_loadTileData(tile_checker, TILE_CHECKER, 1, DMA);
    VDP_loadTileData(tile_lines,   TILE_LINES,   1, DMA);
    VDP_loadTileData(tile_dot,     TILE_DOT,     1, DMA);
    VDP_loadTileData(tile_bar_v,   TILE_BAR_V,   1, DMA);
    VDP_loadTileData(tile_bar_h,   TILE_BAR_H,   1, DMA);
    VDP_loadTileData(tile_bracket, TILE_BRACKET, 1, DMA);
    VDP_loadTileData(tile_diag_a,  TILE_DIAG_A,  1, DMA);
    VDP_loadTileData(tile_diag_b,  TILE_DIAG_B,  1, DMA);
    VDP_loadTileData(tile_cross,   TILE_CROSS,   1, DMA);
    VDP_loadTileData(tile_noise,   TILE_NOISE,   1, DMA);
    VDP_loadTileData(tile_scan,    TILE_SCAN,    1, DMA);
}

static void initPalettes(void)
{
    copyPalette(pal1Hot, pal1Work);
    copyPalette(pal2Cold, pal2Work);
    copyPalette(pal3Ghost, pal3Work);

    PAL_setPalette(PAL0, pal0Text, DMA);
    PAL_setPalette(PAL1, pal1Work, DMA);
    PAL_setPalette(PAL2, pal2Work, DMA);
    PAL_setPalette(PAL3, pal3Work, DMA);
}

static void clearHScroll(void)
{
    u16 i;
    u16 lines = VDP_getScreenHeight();

    if (lines > MAX_LINES)
        lines = MAX_LINES;

    for (i = 0; i < lines; i++)
        hscroll[i] = 0;

    VDP_setHorizontalScrollLine(BG_A, 0, hscroll, lines, DMA_QUEUE);
    VDP_setHorizontalScrollLine(BG_B, 0, hscroll, lines, DMA_QUEUE);
}

static void clearPlanes(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
}

static void putTile(VDPPlane plane, u16 x, u16 y, u16 tile, u16 pal, u16 prio, u16 vflip, u16 hflip)
{
    if ((x >= SCREEN_TILES_X) || (y >= SCREEN_TILES_Y))
        return;

    VDP_setTileMapXY(plane, TILE_ATTR_FULL(pal, prio, vflip, hflip, tile), x, y);
}

static void fillRect(VDPPlane plane, u16 x, u16 y, u16 w, u16 h, u16 tile, u16 pal, u16 prio)
{
    u16 ix;
    u16 iy;

    for (iy = 0; iy < h; iy++)
    {
        for (ix = 0; ix < w; ix++)
            putTile(plane, x + ix, y + iy, tile, pal, prio, FALSE, FALSE);
    }
}

static void drawFrame(void)
{
    u16 x;
    u16 y;

    for (x = 0; x < SCREEN_TILES_X; x++)
    {
        putTile(BG_B, x, 0,  TILE_LINES, PAL3, FALSE, FALSE, FALSE);
        putTile(BG_B, x, 27, TILE_LINES, PAL3, FALSE, FALSE, FALSE);
    }

    for (y = 1; y < 27; y++)
    {
        putTile(BG_B, 0,  y, TILE_BAR_V, PAL3, FALSE, FALSE, FALSE);
        putTile(BG_B, 39, y, TILE_BAR_V, PAL3, FALSE, FALSE, FALSE);
    }
}

static void drawTapetes(void)
{
    fillRect(BG_B, 2,  3,  8,  8, TILE_CHECKER, PAL2, FALSE);
    fillRect(BG_B, 30, 3,  8,  8, TILE_CHECKER, PAL2, FALSE);
    fillRect(BG_B, 2,  20, 8,  6, TILE_LINES,   PAL2, FALSE);
    fillRect(BG_B, 30, 20, 8,  6, TILE_LINES,   PAL2, FALSE);

    fillRect(BG_B, 11, 2, 18, 1, TILE_SCAN, PAL3, FALSE);
    fillRect(BG_B, 11, 25,18, 1, TILE_SCAN, PAL3, FALSE);
}

static void drawStaticTexts(void)
{
    if (currentScene == SCENE_MACRONIC)
    {
        VDP_drawText("MACRONIC DREAM ENGINE", 9, 4);
        VDP_drawText("O CARTUCHO ESTA SONHANDO", 5, 23);
    }
    else if (currentScene == SCENE_MANDALA)
    {
        VDP_drawText("MEGAMANDALA MD", 13, 4);
        VDP_drawText("NADA HA FASE SO FREQUENCIA", 5, 23);
    }
    else if (currentScene == SCENE_TUNNEL)
    {
        VDP_drawText("TUNEL DE SILICIO", 11, 4);
        VDP_drawText("RESPIRA NO PIXEL", 10, 23);
    }
    else if (currentScene == SCENE_GLITCH)
    {
        VDP_drawText("O SINAL ESTA VIVO", 11, 4);
        VDP_drawText("GLITCH CONTROLADO", 10, 23);
    }
    else if (currentScene == SCENE_BREATH)
    {
        VDP_drawText("BOTOP OBSERVA", 13, 4);
        VDP_drawText("CALMA DENTRO DO RUIDO", 8, 23);
    }
    else
    {
        VDP_drawText("BOTOP TA DE OLHO", 12, 4);
        VDP_drawText("NAO DESLIGUE O SONHO", 9, 23);
    }
}

static void drawMandalaCore(u16 f)
{
    s16 cx = 20;
    s16 cy = 14;
    s16 r;
    s16 wobble;
    u16 pal;
    u16 tile;

    fillRect(BG_A, 8, 6, 24, 15, TILE_EMPTY, PAL0, FALSE);

    for (r = 1; r < 11; r++)
    {
        wobble = (s16)(((f >> 2) + (u16)(r * 3)) & 7);
        if (wobble > 3)
            wobble = 7 - wobble;

        pal = (u16)((r + (f >> 4)) & 1) ? PAL1 : PAL3;

        if ((r & 3) == 0)
            tile = TILE_CROSS;
        else if ((r & 1) == 0)
            tile = TILE_DOT;
        else
            tile = TILE_BAR_V;

        putTile(BG_A, (u16)(cx + r),        (u16)cy,             tile, pal, TRUE, FALSE, FALSE);
        putTile(BG_A, (u16)(cx - r),        (u16)cy,             tile, pal, TRUE, FALSE, TRUE);
        putTile(BG_A, (u16)cx,              (u16)(cy + r),       tile, pal, TRUE, TRUE, FALSE);
        putTile(BG_A, (u16)cx,              (u16)(cy - r),       tile, pal, TRUE, FALSE, FALSE);

        putTile(BG_A, (u16)(cx + wobble),   (u16)(cy + r),       TILE_DIAG_A, pal, TRUE, FALSE, FALSE);
        putTile(BG_A, (u16)(cx - wobble),   (u16)(cy - r),       TILE_DIAG_B, pal, TRUE, FALSE, FALSE);
        putTile(BG_A, (u16)(cx + r),        (u16)(cy + wobble),  TILE_BAR_H,  pal, TRUE, FALSE, FALSE);
        putTile(BG_A, (u16)(cx - r),        (u16)(cy - wobble),  TILE_BAR_H,  pal, TRUE, FALSE, TRUE);
    }

    putTile(BG_A, (u16)cx,     (u16)cy,     TILE_CROSS, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)(cx-1), (u16)cy,     TILE_BAR_H, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)(cx+1), (u16)cy,     TILE_BAR_H, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)cx,     (u16)(cy-1), TILE_BAR_V, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)cx,     (u16)(cy+1), TILE_BAR_V, PAL1, TRUE, FALSE, FALSE);
}

static void drawEyeScene(u16 f)
{
    s16 cx = 20;
    s16 cy = 14;
    s16 i;
    s16 d;
    s16 top;
    s16 bottom;
    u16 pupil;
    u16 pal;

    if ((f & 3) != 0)
        return;

    fillRect(BG_A, 5, 7, 30, 14, TILE_EMPTY, PAL0, FALSE);

    for (i = -13; i <= 13; i++)
    {
        d = i;
        if (d < 0)
            d = -d;

        top = cy - 5 + (d >> 2);
        bottom = cy + 5 - (d >> 2);

        pal = ((i + (f >> 4)) & 1) ? PAL2 : PAL3;

        putTile(BG_A, (u16)(cx + i), (u16)top,    TILE_DIAG_A, pal, TRUE, FALSE, FALSE);
        putTile(BG_A, (u16)(cx + i), (u16)bottom, TILE_DIAG_B, pal, TRUE, FALSE, FALSE);

        if ((i & 3) == 0)
        {
            putTile(BG_A, (u16)(cx + i), (u16)(cy - 1), TILE_SCAN, pal, TRUE, FALSE, FALSE);
            putTile(BG_A, (u16)(cx + i), (u16)(cy + 1), TILE_SCAN, pal, TRUE, TRUE, FALSE);
        }
    }

    pupil = 2 + ((f >> 5) & 3);

    for (i = -3; i <= 3; i++)
    {
        if ((i >= -(s16)pupil) && (i <= (s16)pupil))
        {
            putTile(BG_A, (u16)(cx + i), (u16)cy, TILE_CROSS, PAL1, TRUE, FALSE, FALSE);
            putTile(BG_A, (u16)(cx + i), (u16)(cy - 1), TILE_BAR_H, PAL1, TRUE, FALSE, FALSE);
            putTile(BG_A, (u16)(cx + i), (u16)(cy + 1), TILE_BAR_H, PAL1, TRUE, TRUE, FALSE);
        }
    }

    putTile(BG_A, (u16)cx,     (u16)cy,     TILE_DOT,   PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)(cx-1), (u16)cy,     TILE_CROSS, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)(cx+1), (u16)cy,     TILE_CROSS, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)cx,     (u16)(cy-2), TILE_BAR_V, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, (u16)cx,     (u16)(cy+2), TILE_BAR_V, PAL1, TRUE, TRUE, FALSE);

    if ((f & 31) == 0)
    {
        for (i = 0; i < 10; i++)
        {
            u16 gx = 6 + (rng16() % 28);
            u16 gy = 8 + (rng16() % 12);
            u16 gt = (rng16() & 1) ? TILE_NOISE : TILE_DOT;
            u16 gp = (rng16() & 1) ? PAL2 : PAL3;

            putTile(BG_A, gx, gy, gt, gp, TRUE, rng16() & 1, rng16() & 1);
        }
    }
}

static void drawTunnelPattern(void)
{
    u16 x;
    u16 y;
    u16 tile;
    u16 pal;

    for (y = 6; y < 22; y++)
    {
        for (x = 4; x < 36; x++)
        {
            if (((x + y) & 3) == 0)
                tile = TILE_BRACKET;
            else if (((x ^ y) & 7) == 0)
                tile = TILE_DOT;
            else
                tile = TILE_NOISE;

            pal = ((x + y) & 1) ? PAL2 : PAL3;

            putTile(BG_B, x, y, tile, pal, FALSE, FALSE, (x & 1));
        }
    }
}

static void updateLineTunnel(u16 f)
{
    u16 y;
    u16 lines = VDP_getScreenHeight();

    if (lines > MAX_LINES)
        lines = MAX_LINES;

    for (y = 0; y < lines; y++)
    {
        s16 yy = (s16)y - 112;
        s16 wave = (s16)(((y + (f * 3)) & 31));

        if (wave > 15)
            wave = 31 - wave;

        if (yy < 0)
            yy = (s16)-yy;

        hscroll[y] = (s16)((wave - 8) * ((yy >> 5) + 1));
    }

    VDP_setHorizontalScrollLine(BG_B, 0, hscroll, lines, DMA_QUEUE);
}

static void drawGlitchBurst(u16 f)
{
    u16 i;
    u16 x;
    u16 y;
    u16 tile;
    u16 pal;

    if ((f & 3) != 0)
        return;

    for (i = 0; i < 24; i++)
    {
        x = 2 + (rng16() % 36);
        y = 6 + (rng16() % 16);

        switch (rng16() & 7)
        {
            case 0:  tile = TILE_BAR_V;   break;
            case 1:  tile = TILE_BAR_H;   break;
            case 2:  tile = TILE_BRACKET; break;
            case 3:  tile = TILE_DIAG_A;  break;
            case 4:  tile = TILE_DIAG_B;  break;
            case 5:  tile = TILE_CROSS;   break;
            case 6:  tile = TILE_DOT;     break;
            default: tile = TILE_NOISE;   break;
        }

        pal = (rng16() & 1) ? PAL1 : PAL3;

        putTile(BG_A, x, y, tile, pal, TRUE, rng16() & 1, rng16() & 1);
    }
}

static void drawBreathScene(u16 f)
{
    u16 x;
    u16 y;
    u16 tile;
    u16 pal;

    if ((f & 7) != 0)
        return;

    fillRect(BG_A, 5, 7, 30, 14, TILE_EMPTY, PAL0, FALSE);

    for (y = 7; y < 21; y++)
    {
        for (x = 5; x < 35; x++)
        {
            if (((x + y + (f >> 3)) & 7) == 0)
            {
                tile = ((x ^ y) & 1) ? TILE_DOT : TILE_CROSS;
                pal = ((x + f) & 2) ? PAL2 : PAL3;
                putTile(BG_A, x, y, tile, pal, TRUE, FALSE, FALSE);
            }
        }
    }

    putTile(BG_A, 19, 14, TILE_BAR_H, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, 20, 14, TILE_CROSS, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, 21, 14, TILE_BAR_H, PAL1, TRUE, FALSE, TRUE);
    putTile(BG_A, 20, 13, TILE_BAR_V, PAL1, TRUE, FALSE, FALSE);
    putTile(BG_A, 20, 15, TILE_BAR_V, PAL1, TRUE, TRUE, FALSE);
}

static void updatePalettePulse(u16 f)
{
    u16 i;
    u16 pulse = (f >> 3) & 1;

    for (i = 1; i < 16; i++)
    {
        pal1Work[i] = brightenColor(pal1Hot[i], pulse);
        pal2Work[i] = brightenColor(pal2Cold[(i + (f >> 4)) & 15], pulse);
        pal3Work[i] = brightenColor(pal3Ghost[(i + (f >> 5)) & 15], pulse);
    }

    pal1Work[0] = pal1Hot[0];
    pal2Work[0] = pal2Cold[0];
    pal3Work[0] = pal3Ghost[0];

    PAL_setPalette(PAL1, pal1Work, DMA_QUEUE);
    PAL_setPalette(PAL2, pal2Work, DMA_QUEUE);
    PAL_setPalette(PAL3, pal3Work, DMA_QUEUE);
}

static void setupScene(u16 scene)
{
    currentScene = scene % SCENE_COUNT;
    sceneTimer = 0;

    clearPlanes();
    clearHScroll();

    drawFrame();
    drawStaticTexts();

    if (currentScene == SCENE_MACRONIC)
    {
        drawTapetes();
        drawMandalaCore(frameCounter);
    }
    else if (currentScene == SCENE_MANDALA)
    {
        drawTapetes();
        drawMandalaCore(frameCounter);
    }
    else if (currentScene == SCENE_TUNNEL)
    {
        drawTunnelPattern();
    }
    else if (currentScene == SCENE_GLITCH)
    {
        drawTapetes();
        drawMandalaCore(frameCounter);
    }
    else if (currentScene == SCENE_BREATH)
    {
        drawBreathScene(frameCounter);
    }
    else
    {
        drawEyeScene(frameCounter);
    }
}

static void nextScene(void)
{
    setupScene((u16)(currentScene + 1));
}

static void updateScene(void)
{
    if (currentScene == SCENE_MACRONIC)
    {
        if ((frameCounter & 3) == 0)
            drawMandalaCore(frameCounter);
    }
    else if (currentScene == SCENE_MANDALA)
    {
        if ((frameCounter & 1) == 0)
            drawMandalaCore(frameCounter);
    }
    else if (currentScene == SCENE_TUNNEL)
    {
        updateLineTunnel(frameCounter);
    }
    else if (currentScene == SCENE_GLITCH)
    {
        if ((frameCounter & 15) == 0)
            drawMandalaCore(frameCounter);

        drawGlitchBurst(frameCounter);
    }
    else if (currentScene == SCENE_BREATH)
    {
        drawBreathScene(frameCounter);
    }
    else
    {
        drawEyeScene(frameCounter);
    }
}

int main(void)
{
    u16 pad;
    u16 pressed;

    VDP_init();
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32, TRUE);
    VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);

    JOY_init();

    loadTiles();
    initPalettes();

    setupScene(SCENE_MACRONIC);

    while (TRUE)
    {
        pad = JOY_readJoypad(JOY_1);
        pressed = pad & ~previousPad;
        previousPad = pad;

        if (pressed & (BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C))
            nextScene();

        if (sceneTimer > 720)
            nextScene();

        updatePalettePulse(frameCounter);
        updateScene();

        frameCounter++;
        sceneTimer++;

        SYS_doVBlankProcess();
    }

    return 0;
}
