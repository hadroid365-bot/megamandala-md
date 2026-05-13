#include <genesis.h>

/*
    MEGAMANDALA MD
    v0.8 - HYPNOTIC POLISH
    SGDK / Marsdev

    - Sem olho
    - Sem deslizamento de BG
    - Mais preto / mais respiro
    - Psicodelia mais dirigida
    - Flash em assinatura rítmica
    - Glitch localizado
    - START / A / B / C avança cena
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
#define SCENE_DREAM     1
#define SCENE_WAVE      2
#define SCENE_MATRIX    3
#define SCENE_ORACLE    4
#define SCENE_TUNNEL    5
#define SCENE_RAIN      6
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

static const u32 tile_empty[8] = {0,0,0,0,0,0,0,0};

static const u32 tile_dot[8] =
{
    0x00000000,0x00033000,0x00333300,0x03333330,
    0x03333330,0x00333300,0x00033000,0x00000000
};

static const u32 tile_line[8] =
{
    0x11111111,0x00000000,0x22222222,0x00000000,
    0x11111111,0x00000000,0x22222222,0x00000000
};

static const u32 tile_scan[8] =
{
    0x30303030,0x03030303,0x30303030,0x03030303,
    0x30303030,0x03030303,0x30303030,0x03030303
};

static const u32 tile_grid[8] =
{
    0x33333333,0x30000003,0x30022003,0x30022003,
    0x30000003,0x30022003,0x30022003,0x33333333
};

static const u32 tile_noise[8] =
{
    0x10203010,0x03020103,0x30102030,0x02010302,
    0x10203010,0x03020103,0x30102030,0x02010302
};

static const u32 tile_cross[8] =
{
    0x30000003,0x03000030,0x00300300,0x00033000,
    0x00033000,0x00300300,0x03000030,0x30000003
};

static const u32 tile_diag_a[8] =
{
    0x30000000,0x03000000,0x00300000,0x00030000,
    0x00003000,0x00000300,0x00000030,0x00000003
};

static const u32 tile_diag_b[8] =
{
    0x00000003,0x00000030,0x00000300,0x00003000,
    0x00030000,0x00300000,0x03000000,0x30000000
};

static const u32 tile_bar[8] =
{
    0x00333300,0x00333300,0x00333300,0x00333300,
    0x00333300,0x00333300,0x00333300,0x00333300
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
    u16 c = 0x0000;

    /* assinatura rítmica: TA ... TA-TA ... TA ... FLASH */
    if(p < 3)
        c = rgb(7,7,7);
    else if(p == 12 || p == 13)
        c = rgb(7,7,0);
    else if(p == 24 || p == 25 || p == 28 || p == 29)
        c = rgb(7,0,7);
    else if(p == 48 || p == 49)
        c = rgb(0,7,7);
    else if((p >= 96) && (p < 102))
        c = rgb(7,0,0);

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

static void fillRect(VDPPlane plane, u16 x0, u16 y0, u16 w, u16 h, u16 tile, u16 pal)
{
    u16 x, y;

    for(y = y0; y < y0 + h; y++)
        for(x = x0; x < x0 + w; x++)
            put(plane, x, y, tile, pal, 0);
}

static void text1(const char *a, u16 x, u16 y)
{
    VDP_drawText(a, x, y);
}

static void dream(void)
{
    u16 x, y, p, t;
    p = frame >> 3;

    clearPlanes();

    for(y = 1; y < 10; y++)
    {
        for(x = 2; x < 38; x++)
        {
            if(((x + y + p) & 3) == 0)
                t = T_NOISE;
            else if(((x ^ y ^ p) & 7) == 0)
                t = T_GRID;
            else
                t = T_SCAN;

            put(BG_B, x, y, t, ((x + y) & 1) ? PAL1 : PAL2, 0);
        }
    }

    for(y = 18; y < 26; y++)
    {
        for(x = 2; x < 38; x++)
        {
            if(((x * 3 + y + p) & 3) == 0)
                t = T_NOISE;
            else
                t = T_SCAN;

            put(BG_B, x, y, t, PAL1, 0);
        }
    }

    put(BG_A, 18, 14, T_DOT, PAL3, 1);
    put(BG_A, 21, 14, T_DOT, PAL3, 1);
    text1("O CARTUCHO ESTA SONHANDO", 7, 13);
}

static void wave(void)
{
    u16 x, y, p, t;
    p = frame >> 3;

    clearPlanes();

    for(y = 2; y < 26; y++)
    {
        for(x = 4; x < 36; x++)
        {
            if(y > 10 && y < 17)
                continue;

            if(((x + ((y + p) & 7)) & 5) == 0)
                t = T_BAR;
            else if(((x ^ y ^ p) & 7) == 0)
                t = T_SCAN;
            else
                t = T_NOISE;

            put(BG_B, x, y, t, ((x + y) & 1) ? PAL1 : PAL2, 0);
        }
    }

    text1("NAO HA FASE", 14, 12);
    text1("SO FREQUENCIA", 13, 15);
}

static void matrix(void)
{
    u16 x, y, p;
    p = frame >> 2;

    clearPlanes();

    for(y = 0; y < 28; y++)
    {
        for(x = 0; x < 40; x++)
        {
            if(y > 9 && y < 18)
                continue;

            if(((x + p) & 7) == 0)
                put(BG_B, x, y, T_BAR, PAL1, 0);
            else if(((x + y + p) & 31) == 0)
                put(BG_B, x, y, T_DOT, PAL3, 0);
        }
    }

    text1("SINAL EM FLUXO", 10, 13);
}

static void oracle(void)
{
    u16 x, y, p, t;
    p = frame >> 3;

    clearPlanes();

    for(y = 1; y < 26; y++)
    {
        for(x = 1; x < 39; x++)
        {
            if(y == 11 || y == 12 || y == 13 || y == 14 || y == 15)
                continue;

            if(((x * y + p) & 11) == 0)
                t = T_CROSS;
            else if(((x + y + p) & 3) == 0)
                t = T_SCAN;
            else
                continue;

            put(BG_B, x, y, t, ((x ^ y) & 1) ? PAL1 : PAL3, 0);
        }
    }

    /* glitch localizado no canto */
    if((frame & 7) == 0)
    {
        for(y = 2; y < 8; y++)
            for(x = 2; x < 11; x++)
                if(((x + y + frame) & 1) == 0)
                    put(BG_A, x, y, T_NOISE, PAL2, 1);
    }

    text1("NAO E BUG", 15, 12);
    text1("EH HADRONIC", 15, 15);
}

static void tunnel(void)
{
    u16 x, y, p, dist, t;
    p = frame >> 3;

    clearPlanes();

    for(y = 2; y < 26; y++)
    {
        for(x = 2; x < 38; x++)
        {
            if(y > 10 && y < 17)
                continue;

            dist = (x > 20) ? x - 20 : 20 - x;
            dist += (y > 14) ? y - 14 : 14 - y;

            if(((dist + p) & 3) == 0)
                t = T_LINE;
            else if(((x ^ y ^ p) & 7) == 0)
                t = ((x + y) & 1) ? T_DIAG_A : T_DIAG_B;
            else
                continue;

            put(BG_B, x, y, t, ((dist + p) & 1) ? PAL1 : PAL2, 0);
        }
    }

    text1("TUNEL DE FREQUENCIA", 10, 13);
}

static void rain(void)
{
    u16 x, y, p;
    p = frame >> 2;

    clearPlanes();

    for(y = 0; y < 28; y++)
    {
        for(x = 0; x < 40; x++)
        {
            if(y > 8 && y < 19)
                continue;

            if(((x + p) & 7) == 0)
                put(BG_B, x, y, T_DOT, PAL1, 0);
            else if(((x + y + p) & 31) == 0)
                put(BG_B, x, y, T_BAR, PAL3, 0);
        }
    }

    text1("CHUVA ELETRICA", 12, 12);
    text1("O RUIDO TEM MEMORIA", 10, 15);
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

    clearPlanes();

    if(scene == SCENE_BOOT)
    {
        fillRect(BG_B, 0, 0, 40, 28, T_EMPTY, PAL0);
        text1("MEGAMANDALA MD", 13, 11);
        text1("NAO HA FASE", 14, 13);
        text1("SO FREQUENCIA", 13, 14);
    }
}

static void nextScene(void)
{
    setupScene(scene + 1);
}

static void updateScene(void)
{
    pulseFlash(frame);

    if((frame & 3) != 0)
        return;

    if(scene == SCENE_DREAM)
        dream();
    else if(scene == SCENE_WAVE)
        wave();
    else if(scene == SCENE_MATRIX)
        matrix();
    else if(scene == SCENE_ORACLE)
        oracle();
    else if(scene == SCENE_TUNNEL)
        tunnel();
    else if(scene == SCENE_RAIN)
        rain();
    else
    {
        if((frame & 63) < 4)
            PAL_setColor(0, rgb(7,7,7));
        else
            PAL_setColor(0, 0x0000);
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
