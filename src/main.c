#include <genesis.h>

/*
    MEGAMANDALA MD
    v0.5 - FULLSCREEN PULSE RITUAL
    SGDK / Marsdev

    Sem assets externos.
    Sem float.
    Sem malloc.
    Sem outro .c dentro de src/.
*/

#define TILE_BASE       TILE_USER_INDEX

#define T_EMPTY         (TILE_BASE + 0)
#define T_DOT           (TILE_BASE + 1)
#define T_LINE          (TILE_BASE + 2)
#define T_SCAN          (TILE_BASE + 3)
#define T_GRID          (TILE_BASE + 4)
#define T_NOISE         (TILE_BASE + 5)
#define T_CROSS         (TILE_BASE + 6)
#define T_DIAG_A        (TILE_BASE + 7)
#define T_DIAG_B        (TILE_BASE + 8)
#define T_BAR           (TILE_BASE + 9)

#define SCENE_BOOT      0
#define SCENE_MANDALA   1
#define SCENE_TUNNEL    2
#define SCENE_GLITCH    3
#define SCENE_VOID      4
#define SCENE_EYE       5
#define SCENE_COUNT     6

#define SCENE_TIME      720

static u16 frame = 0;
static u16 sceneTimer = 0;
static u16 scene = 0;
static u16 lastJoy = 0;
static u16 seed = 0xB00D;

static u16 pal0[16];
static u16 pal1[16];
static u16 pal2[16];
static u16 pal3[16];

static const u32 tile_empty[8] =
{
    0,0,0,0,0,0,0,0
};

static const u32 tile_dot[8] =
{
    0x00000000,
    0x00033000,
    0x00333300,
    0x03333330,
    0x03333330,
    0x00333300,
    0x00033000,
    0x00000000
};

static const u32 tile_line[8] =
{
    0x11111111,
    0x00000000,
    0x22222222,
    0x00000000,
    0x11111111,
    0x00000000,
    0x22222222,
    0x00000000
};

static const u32 tile_scan[8] =
{
    0x30303030,
    0x03030303,
    0x30303030,
    0x03030303,
    0x30303030,
    0x03030303,
    0x30303030,
    0x03030303
};

static const u32 tile_grid[8] =
{
    0x33333333,
    0x30000003,
    0x30022003,
    0x30022003,
    0x30000003,
    0x30022003,
    0x30022003,
    0x33333333
};

static const u32 tile_noise[8] =
{
    0x10203010,
    0x03020103,
    0x30102030,
    0x02010302,
    0x10203010,
    0x03020103,
    0x30102030,
    0x02010302
};

static const u32 tile_cross[8] =
{
    0x30000003,
    0x03000030,
    0x00300300,
    0x00033000,
    0x00033000,
    0x00300300,
    0x03000030,
    0x30000003
};

static const u32 tile_diag_a[8] =
{
    0x30000000,
    0x03000000,
    0x00300000,
    0x00030000,
    0x00003000,
    0x00000300,
    0x00000030,
    0x00000003
};

static const u32 tile_diag_b[8] =
{
    0x00000003,
    0x00000030,
    0x00000300,
    0x00003000,
    0x00030000,
    0x00300000,
    0x03000000,
    0x30000000
};

static const u32 tile_bar[8] =
{
    0x00333300,
    0x00333300,
    0x00333300,
    0x00333300,
    0x00333300,
    0x00333300,
    0x00333300,
    0x00333300
};

static const u16 baseDark[16] =
{
    0x0000,0x0222,0x0444,0x0666,
    0x0888,0x0AAA,0x0CCC,0x0EEE,
    0x000E,0x00AE,0x0E0E,0x0EE0,
    0x0E00,0x0ACE,0x0A0E,0x0EEE
};

static u16 rng(void)
{
    seed = (u16)(seed * 109u + 89u);
    return seed;
}

static u16 rgb(u16 r, u16 g, u16 b)
{
    return (u16)((b << 9) | (g << 5) | (r << 1));
}

static void loadTiles(void)
{
    VDP_loadTileData(tile_empty, T_EMPTY, 1, DMA);
    VDP_loadTileData(tile_dot,   T_DOT,   1, DMA);
    VDP_loadTileData(tile_line,  T_LINE,  1, DMA);
    VDP_loadTileData(tile_scan,  T_SCAN,  1, DMA);
    VDP_loadTileData(tile_grid,  T_GRID,  1, DMA);
    VDP_loadTileData(tile_noise, T_NOISE, 1, DMA);
    VDP_loadTileData(tile_cross, T_CROSS, 1, DMA);
    VDP_loadTileData(tile_diag_a,T_DIAG_A,1, DMA);
    VDP_loadTileData(tile_diag_b,T_DIAG_B,1, DMA);
    VDP_loadTileData(tile_bar,   T_BAR,   1, DMA);
}

static void setPalettes(u16 mode)
{
    u16 i;

    for(i = 0; i < 16; i++)
    {
        pal0[i] = baseDark[i];
        pal1[i] = baseDark[(i + 3 + mode) & 15];
        pal2[i] = baseDark[(i + 7 + mode) & 15];
        pal3[i] = baseDark[(i + 11 + mode) & 15];
    }

    PAL_setPalette(PAL0, pal0, DMA);
    PAL_setPalette(PAL1, pal1, DMA);
    PAL_setPalette(PAL2, pal2, DMA);
    PAL_setPalette(PAL3, pal3, DMA);
}

static void flashPalette(u16 f)
{
    u16 flash = f & 63;

    if(flash < 3)
    {
        pal0[0] = rgb(7,7,0);
        pal1[0] = rgb(7,0,7);
        pal2[0] = rgb(0,7,7);
        pal3[0] = rgb(7,7,7);
    }
    else if(flash == 8 || flash == 9)
    {
        pal0[0] = rgb(7,0,0);
        pal1[0] = rgb(0,7,0);
        pal2[0] = rgb(0,0,7);
        pal3[0] = rgb(7,7,0);
    }
    else
    {
        pal0[0] = 0x0000;
        pal1[0] = 0x0000;
        pal2[0] = 0x0000;
        pal3[0] = 0x0000;
    }

    PAL_setPalette(PAL0, pal0, DMA_QUEUE);
    PAL_setPalette(PAL1, pal1, DMA_QUEUE);
    PAL_setPalette(PAL2, pal2, DMA_QUEUE);
    PAL_setPalette(PAL3, pal3, DMA_QUEUE);
}

static void put(VDPPlane plane, u16 x, u16 y, u16 tile, u16 pal, u16 prio)
{
    if(x < 40 && y < 28)
        VDP_setTileMapXY(plane, TILE_ATTR_FULL(pal, prio, 0, 0, tile), x, y);
}

static void clearPlanes(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
}

static void fillPlane(VDPPlane plane, u16 tile, u16 pal)
{
    u16 x, y;

    for(y = 0; y < 28; y++)
        for(x = 0; x < 40; x++)
            put(plane, x, y, tile, pal, 0);
}

static void drawFrame(void)
{
    u16 x, y;

    for(x = 0; x < 40; x++)
    {
        put(BG_B, x, 0,  T_LINE, PAL3, 0);
        put(BG_B, x, 27, T_LINE, PAL3, 0);
    }

    for(y = 1; y < 27; y++)
    {
        put(BG_B, 0,  y, T_BAR, PAL3, 0);
        put(BG_B, 39, y, T_BAR, PAL3, 0);
    }
}

static void drawFlashField(u16 f)
{
    u16 x, y;
    u16 tile;

    if((f & 31) < 4)
    {
        tile = ((f >> 2) & 1) ? T_SCAN : T_NOISE;
        fillPlane(BG_B, tile, PAL1);
    }
    else if((f & 63) == 20)
    {
        for(y = 0; y < 28; y++)
            for(x = 0; x < 40; x++)
                put(BG_B, x, y, ((x + y) & 1) ? T_GRID : T_SCAN, PAL2, 0);
    }
}

static void drawMandala(u16 f)
{
    s16 cx = 20;
    s16 cy = 14;
    s16 r;
    s16 w;

    VDP_clearPlane(BG_A, FALSE);

    for(r = 1; r < 12; r++)
    {
        w = (s16)(((f >> 3) + r) & 7);
        if(w > 3) w = 7 - w;

        put(BG_A, cx + r, cy, T_DOT, PAL1, 1);
        put(BG_A, cx - r, cy, T_DOT, PAL1, 1);
        put(BG_A, cx, cy + r, T_DOT, PAL1, 1);
        put(BG_A, cx, cy - r, T_DOT, PAL1, 1);

        put(BG_A, cx + w, cy + r, T_DIAG_A, PAL3, 1);
        put(BG_A, cx - w, cy - r, T_DIAG_B, PAL3, 1);
        put(BG_A, cx + r, cy + w, T_LINE, PAL2, 1);
        put(BG_A, cx - r, cy - w, T_LINE, PAL2, 1);
    }

    put(BG_A, 20, 14, T_CROSS, PAL1, 1);
}

static void drawTunnel(u16 f)
{
    u16 x, y;

    VDP_clearPlane(BG_A, FALSE);

    for(y = 3; y < 25; y++)
    {
        for(x = 6; x < 34; x++)
        {
            if(((x + y + (f >> 3)) & 7) == 0)
                put(BG_A, x, y, T_DOT, PAL1, 1);
            else if(((x ^ y ^ (f >> 4)) & 15) == 0)
                put(BG_A, x, y, T_CROSS, PAL3, 1);
        }
    }

    VDP_drawText("TUNEL DE SILICIO", 11, 4);
}

static void drawGlitch(u16 f)
{
    u16 i, x, y, t, p;

    if((f & 3) != 0) return;

    for(i = 0; i < 48; i++)
    {
        x = rng() % 40;
        y = rng() % 28;

        switch(rng() & 7)
        {
            case 0: t = T_SCAN; break;
            case 1: t = T_GRID; break;
            case 2: t = T_NOISE; break;
            case 3: t = T_CROSS; break;
            case 4: t = T_DIAG_A; break;
            case 5: t = T_DIAG_B; break;
            case 6: t = T_BAR; break;
            default: t = T_DOT; break;
        }

        p = (rng() & 3);
        put(BG_A, x, y, t, p, 1);
    }

    VDP_drawText("NAO E BUG", 15, 4);
    VDP_drawText("E ORACULO", 15, 23);
}

static void drawVoid(u16 f)
{
    u16 x, y;

    VDP_clearPlane(BG_A, FALSE);

    for(y = 7; y < 21; y++)
    {
        for(x = 5; x < 35; x++)
        {
            if(((x + y + (f >> 4)) & 11) == 0)
                put(BG_A, x, y, ((x ^ y) & 1) ? T_DOT : T_CROSS, PAL3, 1);
        }
    }

    VDP_drawText("RESPIRA NO PIXEL", 11, 4);
    VDP_drawText("O SINAL ESTA VIVO", 11, 23);
}

static void drawEye(u16 f)
{
    s16 cx = 20;
    s16 cy = 14;
    s16 i;
    s16 d;
    s16 top;
    s16 bot;
    u16 pulse = 2 + ((f >> 5) & 3);

    VDP_clearPlane(BG_A, FALSE);

    for(i = -13; i <= 13; i++)
    {
        d = i;
        if(d < 0) d = -d;

        top = cy - 5 + (d >> 2);
        bot = cy + 5 - (d >> 2);

        put(BG_A, cx + i, top, T_DIAG_A, PAL2, 1);
        put(BG_A, cx + i, bot, T_DIAG_B, PAL2, 1);

        if((i & 3) == 0)
            put(BG_A, cx + i, cy, T_SCAN, PAL3, 1);
    }

    for(i = -3; i <= 3; i++)
    {
        if(i >= -(s16)pulse && i <= (s16)pulse)
        {
            put(BG_A, cx + i, cy, T_CROSS, PAL1, 1);
            put(BG_A, cx + i, cy - 1, T_LINE, PAL1, 1);
            put(BG_A, cx + i, cy + 1, T_LINE, PAL1, 1);
        }
    }

    VDP_drawText("BOTOP TA DE OLHO", 12, 4);
    VDP_drawText("NAO DESLIGUE O SONHO", 9, 23);
}

static void setupScene(u16 s)
{
    scene = s % SCENE_COUNT;
    sceneTimer = 0;

    clearPlanes();
    setPalettes(scene);

    if(scene == SCENE_BOOT)
    {
        fillPlane(BG_B, T_EMPTY, PAL0);
        drawFrame();
        VDP_drawText("MEGAMANDALA MD", 13, 11);
        VDP_drawText("NAO HA FASE", 14, 13);
        VDP_drawText("SO FREQUENCIA", 13, 14);
    }
    else if(scene == SCENE_MANDALA)
    {
        fillPlane(BG_B, T_GRID, PAL2);
        drawFrame();
        VDP_drawText("MEGAMANDALA MD", 13, 4);
    }
    else if(scene == SCENE_TUNNEL)
    {
        fillPlane(BG_B, T_NOISE, PAL2);
        drawFrame();
    }
    else if(scene == SCENE_GLITCH)
    {
        fillPlane(BG_B, T_SCAN, PAL1);
    }
    else if(scene == SCENE_VOID)
    {
        fillPlane(BG_B, T_EMPTY, PAL0);
        drawFrame();
    }
    else
    {
        fillPlane(BG_B, T_LINE, PAL2);
        drawFrame();
    }
}

static void nextScene(void)
{
    setupScene(scene + 1);
}

static void updateScene(void)
{
    flashPalette(frame);

    if(scene != SCENE_BOOT)
        drawFlashField(frame);

    if(scene == SCENE_BOOT)
    {
        if((frame & 63) < 4)
            fillPlane(BG_A, T_SCAN, PAL1);
        else
            VDP_clearPlane(BG_A, FALSE);
    }
    else if(scene == SCENE_MANDALA)
        drawMandala(frame);
    else if(scene == SCENE_TUNNEL)
        drawTunnel(frame);
    else if(scene == SCENE_GLITCH)
        drawGlitch(frame);
    else if(scene == SCENE_VOID)
        drawVoid(frame);
    else
        drawEye(frame);
}

int main(bool hard)
{
    u16 joy;
    u16 pressed;

    VDP_init();
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32, TRUE);

    JOY_init();

    loadTiles();
    setPalettes(0);
    setupScene(SCENE_BOOT);

    while(TRUE)
    {
        joy = JOY_readJoypad(JOY_1);
        pressed = joy & ~lastJoy;
        lastJoy = joy;

        if(pressed & (BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C))
            nextScene();

        if(sceneTimer > SCENE_TIME)
            nextScene();

        updateScene();

        if(scene == SCENE_TUNNEL)
        {
            VDP_setHorizontalScroll(BG_B, frame);
            VDP_setVerticalScroll(BG_B, frame >> 2);
        }
        else if(scene == SCENE_GLITCH)
        {
            VDP_setHorizontalScroll(BG_B, frame << 1);
            VDP_setVerticalScroll(BG_B, frame >> 1);
        }
        else
        {
            VDP_setHorizontalScroll(BG_B, frame >> 3);
            VDP_setVerticalScroll(BG_B, 0);
        }

        frame++;
        sceneTimer++;

        SYS_doVBlankProcess();
    }

    return 0;
}
