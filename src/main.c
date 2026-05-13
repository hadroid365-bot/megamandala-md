#include <genesis.h>

/*
    MEGAMANDALA MD
    v0.6 - STATIC BG / FULLSCREEN PULSE
    SGDK / Marsdev

    Ideia:
    - Backgrounds permanecem na tela.
    - Sem deslizamento de BG.
    - Textos fixos/rituais.
    - Artefatos reduzidos.
    - Flashs fullscreen mais fortes por paleta.
    - START / A / B / C avança cena.
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
#define SCENE_RESPIRA   1
#define SCENE_GRADE     2
#define SCENE_CIRCUITO  3
#define SCENE_MATRIZ    4
#define SCENE_ORACULO   5
#define SCENE_EYE       6
#define SCENE_COUNT     7

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

static void pulseFlash(u16 f)
{
    u16 p = f & 127;
    u16 c;

    if(p < 3)
        c = rgb(7,7,7);
    else if(p == 8 || p == 9)
        c = rgb(7,7,0);
    else if(p == 16 || p == 17)
        c = rgb(7,0,7);
    else if(p == 24 || p == 25)
        c = rgb(0,7,7);
    else if((p >= 64) && (p < 68))
        c = rgb(7,0,0);
    else
        c = 0x0000;

    pal0[0] = c;
    pal1[0] = c;
    pal2[0] = c;
    pal3[0] = c;

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
    {
        for(x = 0; x < 40; x++)
            put(plane, x, y, tile, pal, 0);
    }
}

static void drawTextPair(const char *a, const char *b)
{
    VDP_drawText(a, 12, 10);
    VDP_drawText(b, 9, 16);
}

static void drawSoftFrame(u16 pal)
{
    u16 x, y;

    for(x = 3; x < 37; x++)
    {
        put(BG_B, x, 3,  T_DOT, pal, 0);
        put(BG_B, x, 24, T_DOT, pal, 0);
    }

    for(y = 4; y < 24; y++)
    {
        put(BG_B, 3,  y, T_DOT, pal, 0);
        put(BG_B, 36, y, T_DOT, pal, 0);
    }
}

static void bgRespira(void)
{
    clearPlanes();
    fillPlane(BG_B, T_EMPTY, PAL0);
    drawSoftFrame(PAL2);
    drawTextPair("TA DE OLHO", "NAO DESLIGUE O SONHO");
}

static void bgGrade(void)
{
    clearPlanes();
    fillPlane(BG_B, T_EMPTY, PAL0);

    {
        u16 x, y;
        for(y = 4; y < 24; y++)
        {
            for(x = 4; x < 36; x++)
            {
                if(((x + y) & 3) == 0)
                    put(BG_B, x, y, T_DOT, PAL2, 0);
                else if(((x ^ y) & 7) == 0)
                    put(BG_B, x, y, T_GRID, PAL3, 0);
            }
        }
    }

    drawSoftFrame(PAL3);
    drawTextPair("TA DE OLHO", "NAO DESLIGUE O SONHO");
}

static void bgCircuito(void)
{
    clearPlanes();
    fillPlane(BG_B, T_EMPTY, PAL0);

    {
        u16 x, y;
        for(y = 5; y < 23; y++)
        {
            for(x = 2; x < 38; x++)
            {
                if((y == 6) || (y == 21))
                    put(BG_B, x, y, T_LINE, PAL2, 0);
                else if(((x + (y * 3)) & 11) == 0)
                    put(BG_B, x, y, T_NOISE, PAL3, 0);
            }
        }
    }

    drawTextPair("TA DE OLHO", "NAO DESLIGUE O SONHO");
}

static void bgMatriz(void)
{
    clearPlanes();

    {
        u16 x, y;
        for(y = 0; y < 28; y++)
        {
            for(x = 0; x < 40; x++)
            {
                if((x & 1) == 0)
                    put(BG_B, x, y, T_BAR, PAL1, 0);
                else
                    put(BG_B, x, y, T_EMPTY, PAL0, 0);
            }
        }
    }

    drawTextPair("TA DE OLHO", "NAO DESLIGUE O SONHO");
}

static void bgOraculo(void)
{
    clearPlanes();
    fillPlane(BG_B, T_EMPTY, PAL0);

    {
        u16 x, y;
        for(y = 4; y < 24; y++)
        {
            for(x = 4; x < 36; x++)
            {
                if(((x + y) & 1) == 0)
                    put(BG_B, x, y, T_SCAN, PAL1, 0);
                if(((x * y) & 31) == 0)
                    put(BG_B, x, y, T_CROSS, PAL3, 0);
            }
        }
    }

    VDP_drawText("NAO E BUG", 15, 10);
    VDP_drawText("E ORACULO", 15, 16);
}

static void drawEyeOnce(void)
{
    s16 cx = 20;
    s16 cy = 14;
    s16 i;
    s16 d;
    s16 top;
    s16 bot;

    clearPlanes();
    fillPlane(BG_B, T_EMPTY, PAL0);

    for(i = -13; i <= 13; i++)
    {
        d = i;
        if(d < 0) d = -d;

        top = cy - 5 + (d >> 2);
        bot = cy + 5 - (d >> 2);

        put(BG_A, cx + i, top, T_DIAG_A, PAL3, 1);
        put(BG_A, cx + i, bot, T_DIAG_B, PAL3, 1);

        if((i & 3) == 0)
            put(BG_A, cx + i, cy, T_SCAN, PAL2, 1);
    }

    put(BG_A, 18, 14, T_CROSS, PAL1, 1);
    put(BG_A, 19, 14, T_CROSS, PAL1, 1);
    put(BG_A, 20, 14, T_DOT,   PAL1, 1);
    put(BG_A, 21, 14, T_CROSS, PAL1, 1);
    put(BG_A, 22, 14, T_CROSS, PAL1, 1);

    drawTextPair("TA DE OLHO", "NAO DESLIGUE O SONHO");
}

static void smallSparks(void)
{
    u16 i, x, y;

    if((frame & 15) != 0)
        return;

    for(i = 0; i < 5; i++)
    {
        x = 4 + (rng() % 32);
        y = 5 + (rng() % 18);
        put(BG_A, x, y, (rng() & 1) ? T_DOT : T_CROSS, (rng() & 1) ? PAL2 : PAL3, 1);
    }
}

static void setupScene(u16 s)
{
    scene = s % SCENE_COUNT;
    sceneTimer = 0;

    setPalettes(scene);

    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    VDP_setVerticalScroll(BG_A, 0);
    VDP_setVerticalScroll(BG_B, 0);

    if(scene == SCENE_BOOT)
    {
        clearPlanes();
        fillPlane(BG_B, T_EMPTY, PAL0);
        VDP_drawText("MEGAMANDALA MD", 13, 11);
        VDP_drawText("NÃO HA FASE", 14, 13);
        VDP_drawText("SO FREQUENCIA", 13, 14);
    }
    else if(scene == SCENE_RESPIRA)
        bgRespira();
    else if(scene == SCENE_GRADE)
        bgGrade();
    else if(scene == SCENE_CIRCUITO)
        bgCircuito();
    else if(scene == SCENE_MATRIZ)
        bgMatriz();
    else if(scene == SCENE_ORACULO)
        bgOraculo();
    else
        drawEyeOnce();
}

static void nextScene(void)
{
    setupScene(scene + 1);
}

static void updateScene(void)
{
    pulseFlash(frame);

    if(scene == SCENE_BOOT)
    {
        if((frame & 63) < 4)
            PAL_setColor(0, rgb(7,7,7));
        else
            PAL_setColor(0, 0x0000);
    }
    else
    {
        smallSparks();
    }
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

        frame++;
        sceneTimer++;

        SYS_doVBlankProcess();
    }

    return 0;
}
