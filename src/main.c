#include <genesis.h>

/*
    MEGADRONIC
    v1.4 - SIGNAL MEMORY LITE

    Base: v1.3 estavel
    Adiciona:
    - LFSR no lugar do RNG multiplicativo
    - C segurado aumenta corruption_level
    - C solto reduz corruption_level aos poucos
    - Cenas ORACLE / V02 / V03GLITCH / BREATH deixam rastro
    - Pause vivo: a imagem para, mas o sinal ainda pulsa
    - HUD: SPD / SCN / COR / AUTO / PAUSE / HYPER

    Sem H-INT.
    Sem dirty buffer real.
    Sem __builtin_ctz.
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
#define T_CHECKER       (TILE_BASE + 10)
#define T_BAR_H         (TILE_BASE + 11)
#define T_BRACKET       (TILE_BASE + 12)

#define MAX_LINES       240

#define SCENE_DREAM     0
#define SCENE_WAVE      1
#define SCENE_MATRIX    2
#define SCENE_ORACLE    3
#define SCENE_TUNNEL    4
#define SCENE_RAIN      5
#define SCENE_V02       6
#define SCENE_MACRONIC  7
#define SCENE_V03TUNNEL 8
#define SCENE_V03GLITCH 9
#define SCENE_BREATH    10
#define SCENE_COUNT     11

#define BOOT_LOGO       0
#define BOOT_SIGNAL     1
#define BOOT_PRESS      2

#define BOOT_TIME       120
#define SCENE_TIME      1200

static u16 frame = 0;
static u16 animFrame = 0;
static u16 animSpeed = 1;
static u16 sceneTimer = 0;
static u16 scene = 0;
static u16 lastJoy = 0;

/* v1.4: LFSR seed */
static u16 seed = 0xACE1u;

/* v1.4: signal control */
static u16 corruption_level = 0;
static u16 hyperFlash = FALSE;
static u16 paused = FALSE;
static u16 autoMode = TRUE;

static u16 inBoot = TRUE;
static u16 bootStage = BOOT_LOGO;
static u16 bootTimer = 0;
static u16 lastBootStage = 999;
static u16 bootFlash = 2;

static s16 hscroll[MAX_LINES];

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

static const u32 tile_checker[8] =
{
    0x10101010,0x01010101,0x10101010,0x01010101,
    0x10101010,0x01010101,0x10101010,0x01010101
};

static const u32 tile_bar_h[8] =
{
    0x00000000,0x00000000,0x33333333,0x33333333,
    0x00000000,0x00000000,0x11111111,0x00000000
};

static const u32 tile_bracket[8] =
{
    0x03333300,0x03000000,0x03000000,0x03330000,
    0x03330000,0x03000000,0x03000000,0x03333300
};

static const u16 baseDark[16] =
{
    0x0000,0x0222,0x0444,0x0666,
    0x0888,0x0AAA,0x0CCC,0x0EEE,
    0x000E,0x00AE,0x0E0E,0x0EE0,
    0x0E00,0x0ACE,0x0A0E,0x0EEE
};

/* ---------------- CORE UTILS ---------------- */

static u16 fast_noise(void)
{
    seed = (u16)((seed >> 1) ^ ((-(seed & 1u)) & 0xB400u));
    return seed;
}

#define rng() fast_noise()

static u16 rgb(u16 r, u16 g, u16 b)
{
    return (u16)((b << 9) | (g << 5) | (r << 1));
}

static void put(VDPPlane plane, u16 x, u16 y, u16 tile, u16 pal, u16 prio)
{
    if(x < 40 && y < 28)
        VDP_setTileMapXY(plane, TILE_ATTR_FULL(pal, prio, 0, 0, tile), x, y);
}

static void putFlip(VDPPlane plane, u16 x, u16 y, u16 tile, u16 pal, u16 prio, u16 vf, u16 hf)
{
    if(x < 40 && y < 28)
        VDP_setTileMapXY(plane, TILE_ATTR_FULL(pal, prio, vf, hf, tile), x, y);
}

static void clearPlanes(void)
{
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
}

static void text1(const char *a, u16 x, u16 y)
{
    VDP_drawText(a, x, y);
}

static void clearTextLine(u16 y)
{
    VDP_drawText("                                        ", 0, y);
}

static void loadTiles(void)
{
    VDP_loadTileData(tile_empty,   T_EMPTY,   1, DMA);
    VDP_loadTileData(tile_dot,     T_DOT,     1, DMA);
    VDP_loadTileData(tile_line,    T_LINE,    1, DMA);
    VDP_loadTileData(tile_scan,    T_SCAN,    1, DMA);
    VDP_loadTileData(tile_grid,    T_GRID,    1, DMA);
    VDP_loadTileData(tile_noise,   T_NOISE,   1, DMA);
    VDP_loadTileData(tile_cross,   T_CROSS,   1, DMA);
    VDP_loadTileData(tile_diag_a,  T_DIAG_A,  1, DMA);
    VDP_loadTileData(tile_diag_b,  T_DIAG_B,  1, DMA);
    VDP_loadTileData(tile_bar,     T_BAR,     1, DMA);
    VDP_loadTileData(tile_checker, T_CHECKER, 1, DMA);
    VDP_loadTileData(tile_bar_h,   T_BAR_H,   1, DMA);
    VDP_loadTileData(tile_bracket, T_BRACKET, 1, DMA);
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

static void clearHScroll(void)
{
    u16 i;
    u16 lines = VDP_getScreenHeight();

    if(lines > MAX_LINES)
        lines = MAX_LINES;

    for(i = 0; i < lines; i++)
        hscroll[i] = 0;

    VDP_setHorizontalScrollLine(BG_A, 0, hscroll, lines, DMA_QUEUE);
    VDP_setHorizontalScrollLine(BG_B, 0, hscroll, lines, DMA_QUEUE);
}

/*
    v1.4: dissolve parcial seletivo.
    Não substitui clearPlanes().
    Só é chamado em cenas com memória, e só a cada redraw.
*/
static void dissolve_partial(u16 intensity)
{
    u16 x;
    u16 y;
    u16 threshold;

    if(intensity == 0)
        return;

    if(intensity > 180)
        threshold = 180;
    else
        threshold = intensity;

    for(y = 0; y < 28; y++)
    {
        for(x = 0; x < 40; x++)
        {
            if((fast_noise() & 0xFF) < threshold)
                VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, 0, 0, 0, T_EMPTY), x, y);
        }
    }
}

/* ---------------- FLASH / BOOT ---------------- */

static void pulseFlash(u16 f)
{
    u16 p = f & 127;
    u16 c = 0x0000;

    if(hyperFlash)
    {
        if((f & 3) == 0)
            c = rgb(7,7,7);
        else if((f & 7) == 1)
            c = rgb(7,0,7);
        else if((f & 7) == 3)
            c = rgb(0,7,7);
        else if((f & 15) == 7)
            c = rgb(7,0,0);

        if(corruption_level > 180 && ((f & 5) == 0))
            c = rgb(7,7,0);
    }
    else
    {
        if(p < 2)
            c = rgb(7,7,7);
        else if(p == 12)
            c = rgb(7,7,0);
        else if(p == 24 || p == 28)
            c = rgb(7,0,7);
        else if(p == 48)
            c = rgb(0,7,7);
        else if((p >= 96) && (p < 100))
            c = rgb(7,0,0);
    }

    pal0[0] = c;
    pal1[0] = c;
    pal2[0] = c;
    pal3[0] = c;

    PAL_setPalette(PAL0, pal0, DMA_QUEUE);
    PAL_setPalette(PAL1, pal1, DMA_QUEUE);
    PAL_setPalette(PAL2, pal2, DMA_QUEUE);
    PAL_setPalette(PAL3, pal3, DMA_QUEUE);
}

static void bootPulse(void)
{
    if(bootFlash > 0)
    {
        PAL_setColor(0, rgb(7,7,7));
        bootFlash--;
    }
    else
    {
        PAL_setColor(0, 0x0000);
    }
}

static void drawBootStage(void)
{
    if(lastBootStage != bootStage)
    {
        clearPlanes();
        lastBootStage = bootStage;
        bootFlash = 2;
    }

    bootPulse();

    if(bootStage == BOOT_LOGO)
    {
        text1("MEGADRONIC", 15, 13);
    }
    else if(bootStage == BOOT_SIGNAL)
    {
        text1("ONLY SIGNAL", 14, 13);
    }
    else
    {
        if((frame & 63) < 48)
            text1("PRESS START", 14, 13);
        else
            clearTextLine(13);
    }

    if((frame & 31) == 0)
    {
        u16 x = rng() % 40;
        u16 y = rng() % 28;

        if((x < 4) || (x > 35) || (y < 4) || (y > 23))
            put(BG_A, x, y, T_DOT, PAL3, 1);
    }
}

static void startSignal(void)
{
    inBoot = FALSE;
    scene = SCENE_DREAM;
    sceneTimer = 0;
    animFrame = 0;
    corruption_level = 0;
    setPalettes(scene);
    clearPlanes();
    clearHScroll();
    PAL_setColor(0, rgb(7,7,7));
}

static void updateBoot(u16 pressed)
{
    if(pressed & (BUTTON_START | BUTTON_A | BUTTON_B | BUTTON_C))
    {
        startSignal();
        return;
    }

    drawBootStage();

    if(bootStage < BOOT_PRESS)
    {
        bootTimer++;

        if(bootTimer > BOOT_TIME)
        {
            bootTimer = 0;
            bootStage++;
        }
    }
}

/* ---------------- CLEAN SCENES ---------------- */

static void dream(void)
{
    u16 x,y,p,t;
    p = animFrame >> 3;
    clearPlanes();

    for(y=1;y<10;y++)
        for(x=2;x<38;x++)
        {
            if(((x+y+p)&3)==0) t=T_NOISE;
            else if(((x^y^p)&7)==0) t=T_GRID;
            else t=T_SCAN;
            put(BG_B,x,y,t,((x+y)&1)?PAL1:PAL2,0);
        }

    for(y=18;y<26;y++)
        for(x=2;x<38;x++)
        {
            if(((x*3+y+p)&3)==0) t=T_NOISE;
            else t=T_SCAN;
            put(BG_B,x,y,t,PAL1,0);
        }

    text1("O CARTUCHO ESTA SONHANDO",7,13);
}

static void wave(void)
{
    u16 x,y,p,t;
    p = animFrame >> 3;
    clearPlanes();

    for(y=2;y<26;y++)
        for(x=4;x<36;x++)
        {
            if(y>10 && y<17) continue;

            if(((x+((y+p)&7))&5)==0) t=T_BAR;
            else if(((x^y^p)&7)==0) t=T_SCAN;
            else t=T_NOISE;

            put(BG_B,x,y,t,((x+y)&1)?PAL1:PAL2,0);
        }

    text1("NAO HA FASE",14,12);
    text1("SO FREQUENCIA",13,15);
}

static void matrix(void)
{
    u16 x,y,p;
    p = animFrame >> 2;
    clearPlanes();

    for(y=0;y<28;y++)
        for(x=0;x<40;x++)
        {
            if(y>9 && y<18) continue;

            if(((x+p)&7)==0)
                put(BG_B,x,y,T_BAR,PAL1,0);
            else if(((x+y+p)&31)==0)
                put(BG_B,x,y,T_DOT,PAL3,0);
        }

    text1("O SINAL ESTA ABERTO",10,13);
}

static void tunnel(void)
{
    u16 x,y,p,dist,t;
    p = animFrame >> 3;
    clearPlanes();

    for(y=2;y<26;y++)
        for(x=2;x<38;x++)
        {
            if(y>10 && y<17) continue;

            dist = (x>20)?x-20:20-x;
            dist += (y>14)?y-14:14-y;

            if(((dist+p)&3)==0) t=T_LINE;
            else if(((x^y^p)&7)==0) t=((x+y)&1)?T_DIAG_A:T_DIAG_B;
            else continue;

            put(BG_B,x,y,t,((dist+p)&1)?PAL1:PAL2,0);
        }

    text1("TUNEL DE FREQUENCIA",10,13);
}

static void rain(void)
{
    u16 x,y,p;
    p = animFrame >> 2;
    clearPlanes();

    for(y=0;y<28;y++)
        for(x=0;x<40;x++)
        {
            if(y>8 && y<19) continue;

            if(((x+p)&7)==0)
                put(BG_B,x,y,T_DOT,PAL1,0);
            else if(((x+y+p)&31)==0)
                put(BG_B,x,y,T_BAR,PAL3,0);
        }

    text1("CHUVA ELETRICA",12,12);
    text1("O RUIDO TEM MEMORIA",10,15);
}

static void macronic(void)
{
    u16 x,y,p,t;
    p = animFrame >> 2;
    clearPlanes();

    for(y=3;y<11;y++)
        for(x=2;x<10;x++)
            put(BG_B,x,y,((x+y+p)&1)?T_CHECKER:T_LINE,PAL2,0);

    for(y=3;y<11;y++)
        for(x=30;x<38;x++)
            put(BG_B,x,y,((x^y^p)&1)?T_CHECKER:T_SCAN,PAL2,0);

    for(y=20;y<26;y++)
        for(x=2;x<10;x++)
            put(BG_B,x,y,((x+p)&1)?T_LINE:T_SCAN,PAL2,0);

    for(y=20;y<26;y++)
        for(x=30;x<38;x++)
            put(BG_B,x,y,((y+p)&1)?T_LINE:T_CHECKER,PAL2,0);

    for(x=11;x<29;x++)
    {
        put(BG_B,x,2,T_SCAN,PAL3,0);
        put(BG_B,x,25,T_SCAN,PAL3,0);
    }

    for(y=7;y<21;y++)
        for(x=12;x<28;x++)
        {
            if(((x*y+p)&15)==0) t=T_CROSS;
            else if(((x+y+p)&7)==0) t=T_DOT;
            else continue;

            put(BG_A,x,y,t,((x+y)&1)?PAL1:PAL3,1);
        }

    text1("MACRONIC DREAM ENGINE",9,4);
    text1("O CARTUCHO ESTA SONHANDO",5,23);
}

/* ---------------- MEMORY SCENES ---------------- */

static void oracle(void)
{
    u16 x,y,p,t;
    p = animFrame >> 3;

    if(corruption_level == 0)
        clearPlanes();
    else
        dissolve_partial(corruption_level >> 2);

    for(y=1;y<26;y++)
        for(x=1;x<39;x++)
        {
            if(y>=11 && y<=15) continue;

            if(((x*y+p)&11)==0) t=T_CROSS;
            else if(((x+y+p)&3)==0) t=T_SCAN;
            else continue;

            put(BG_B,x,y,t,((x^y)&1)?PAL1:PAL3,0);
        }

    if(corruption_level < 120)
    {
        text1("NAO E BUG",15,12);
        text1("E ORACULO",15,15);
    }
    else
    {
        if((frame & 7) < 5)
        {
            text1("NAO ...",15,12);
            text1("E ...",15,15);
        }

        if(corruption_level > 200 && (frame & 15) == 0)
            put(BG_A, 15 + (rng() % 10), 12 + (rng() % 4), T_NOISE, PAL3, 1);
    }
}

static void v02Signal(void)
{
    u16 x,y,p,t,pal;
    s16 textOffset;

    p = animFrame >> 1;

    if(corruption_level == 0)
        clearPlanes();
    else
        dissolve_partial(corruption_level >> 3);

    for(y=0;y<12;y++)
        for(x=0;x<40;x++)
        {
            if(((x+y+p)&1)==0) t=T_NOISE;
            else if(((x^y^p)&3)==0) t=T_GRID;
            else t=T_SCAN;

            pal = ((x+y+p)&2) ? PAL1 : PAL3;
            put(BG_B,x,y,t,pal,0);
        }

    for(y=15;y<28;y++)
        for(x=0;x<40;x++)
        {
            if(((x*3+y+p)&1)==0) t=T_SCAN;
            else if(((x+y+p)&3)==0) t=T_CROSS;
            else t=T_NOISE;

            pal = ((x^y^p)&2) ? PAL2 : PAL1;
            put(BG_B,x,y,t,pal,0);
        }

    textOffset = 0;
    if(corruption_level > 100)
        textOffset = (s16)(rng() & 3) - 1;

    text1("SIGNAL 02", (u16)(15 + textOffset), 13);
}

static void v03Glitch(void)
{
    u16 i,x,y,t,pal;
    u16 density;
    u16 entropyMask;
    u16 vf;
    u16 hf;

    if(corruption_level == 0)
        clearPlanes();
    else
        dissolve_partial(corruption_level >> 1);

    density = 34 + (corruption_level >> 2);
    if(density > 98)
        density = 98;

    entropyMask = (corruption_level > 100) ? 15 : 7;

    for(i=0;i<density;i++)
    {
        x = 2 + (rng() % 36);
        y = 6 + (rng() % 16);

        switch(rng() & entropyMask)
        {
            case 0: t=T_BAR; break;
            case 1: t=T_BAR_H; break;
            case 2: t=T_BRACKET; break;
            case 3: t=T_DIAG_A; break;
            case 4: t=T_DIAG_B; break;
            case 5: t=T_CROSS; break;
            case 6: t=T_DOT; break;
            default:t=T_NOISE; break;
        }

        pal = (rng() & 1)?PAL1:PAL3;

        vf = 0;
        hf = 0;

        if(corruption_level > 150)
        {
            vf = rng() & 1;
            hf = rng() & 1;
        }

        putFlip(BG_A,x,y,t,pal,1,vf,hf);
    }

    if(corruption_level < 200)
        text1("O SINAL ESTA VIVO",11,4);

    if(corruption_level < 150)
        text1("GLITCH CONTROLADO",10,23);
}

static void breath(void)
{
    u16 x,y,p,t,pal;
    p = animFrame >> 3;

    if(corruption_level < 50)
        clearPlanes();
    else
        dissolve_partial(corruption_level >> 3);

    for(y=7;y<21;y++)
        for(x=5;x<35;x++)
        {
            if(((x+y+p)&7)==0)
            {
                t = ((x^y)&1)?T_DOT:T_CROSS;
                pal = ((x+p)&2)?PAL2:PAL3;
                put(BG_A,x,y,t,pal,1);
            }
        }

    put(BG_A,19,14,T_BAR_H,PAL1,1);
    put(BG_A,20,14,T_CROSS,PAL1,1);
    put(BG_A,21,14,T_BAR_H,PAL1,1);
    put(BG_A,20,13,T_BAR,PAL1,1);
    put(BG_A,20,15,T_BAR,PAL1,1);

    if(corruption_level < 100)
    {
        text1("BOTOP OBSERVA",13,4);
        text1("CALMA DENTRO DO RUIDO",8,23);
    }
    else
    {
        if((frame & 15) < 12)
        {
            text1("BOT.. OBSERV.",13,4);
            text1("CAL.. DENTRO ..",8,23);
        }
    }
}

static void v03Tunnel(void)
{
    u16 x,y,p,t,pal;
    u16 lines;

    p = animFrame >> 2;
    clearPlanes();

    for(y=6;y<22;y++)
        for(x=4;x<36;x++)
        {
            if(((x+y+p)&3)==0) t=T_BRACKET;
            else if(((x^y^p)&7)==0) t=T_DOT;
            else t=T_NOISE;

            pal = ((x+y+p)&1)?PAL2:PAL3;
            putFlip(BG_B,x,y,t,pal,0,0,(x&1));
        }

    lines = VDP_getScreenHeight();
    if(lines > MAX_LINES)
        lines = MAX_LINES;

    for(y=0;y<lines;y++)
    {
        s16 yy = (s16)y - 112;
        s16 wave = (s16)((y + (p * 3)) & 31);

        if(wave > 15)
            wave = 31 - wave;

        if(yy < 0)
            yy = -yy;

        hscroll[y] = (s16)((wave - 8) * ((yy >> 5) + 1));

        if(corruption_level > 150)
            hscroll[y] += (s16)(rng() & 7) - 4;
    }

    VDP_setHorizontalScrollLine(BG_B, 0, hscroll, lines, DMA_QUEUE);

    text1("TUNEL DE SILICIO",11,4);
    text1("RESPIRA NO PIXEL",10,23);
}

/* ---------------- HUD / CONTROL ---------------- */

static void drawHUD(void)
{
    char spd[6];
    char scn[7];
    char cor[8];

    spd[0] = 'S'; spd[1] = 'P'; spd[2] = 'D'; spd[3] = ' '; spd[4] = '0' + animSpeed; spd[5] = 0;

    scn[0] = 'S'; scn[1] = 'C'; scn[2] = 'N'; scn[3] = ' ';
    scn[4] = '0' + ((scene + 1) / 10);
    scn[5] = '0' + ((scene + 1) % 10);
    scn[6] = 0;

    cor[0] = 'C'; cor[1] = 'O'; cor[2] = 'R'; cor[3] = ' ';
    cor[4] = '0' + ((corruption_level / 100) % 10);
    cor[5] = '0' + ((corruption_level / 10) % 10);
    cor[6] = '0' + (corruption_level % 10);
    cor[7] = 0;

    VDP_drawText("                                        ", 0, 26);
    VDP_drawText("                                        ", 0, 27);

    VDP_drawText(spd, 0, 26);
    VDP_drawText(scn, 0, 27);
    VDP_drawText(cor, 10, 26);

    if(autoMode)
        VDP_drawText("AUTO", 35, 26);
    else
        VDP_drawText("MAN ", 35, 26);

    if(paused)
        VDP_drawText("PAUSE", 34, 27);
    else if(hyperFlash)
        VDP_drawText("HYPER", 34, 27);
    else
        VDP_drawText("     ", 34, 27);
}

static void drawPauseLife(void)
{
    if((frame & 63) < 32)
    {
        put(BG_A,19,14,T_DOT,PAL3,1);
        put(BG_A,20,14,T_CROSS,PAL1,1);
        put(BG_A,21,14,T_DOT,PAL3,1);
    }

    if((frame & 31) < 20)
        text1("SINAL DORMINDO",12,15);
    else
        clearTextLine(15);
}

static void setupScene(u16 s)
{
    scene = s % SCENE_COUNT;
    sceneTimer = 0;
    setPalettes(scene);
    clearHScroll();
    clearPlanes();
}

static void nextScene(void)
{
    setupScene(scene + 1);
}

static void updateScene(void)
{
    pulseFlash(paused ? frame : animFrame);

    if(paused)
    {
        drawPauseLife();
        drawHUD();
        return;
    }

    if(scene == SCENE_V03TUNNEL)
    {
        v03Tunnel();
        drawHUD();
        return;
    }

    if((frame & 3) != 0)
    {
        drawHUD();
        return;
    }

    if(scene != SCENE_ORACLE &&
       scene != SCENE_V02 &&
       scene != SCENE_V03GLITCH &&
       scene != SCENE_BREATH)
    {
        clearHScroll();
    }

    if(scene == SCENE_DREAM) dream();
    else if(scene == SCENE_WAVE) wave();
    else if(scene == SCENE_MATRIX) matrix();
    else if(scene == SCENE_ORACLE) oracle();
    else if(scene == SCENE_TUNNEL) tunnel();
    else if(scene == SCENE_RAIN) rain();
    else if(scene == SCENE_V02) v02Signal();
    else if(scene == SCENE_MACRONIC) macronic();
    else if(scene == SCENE_V03GLITCH) v03Glitch();
    else breath();

    drawHUD();
}

/* ---------------- MAIN ---------------- */

int main(bool hard)
{
    u16 joy;
    u16 pressed;

    VDP_init();
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64,32,TRUE);
    VDP_setScrollingMode(HSCROLL_LINE,VSCROLL_PLANE);

    JOY_init();

    loadTiles();
    setPalettes(0);
    clearPlanes();
    clearHScroll();

    while(TRUE)
    {
        joy = JOY_readJoypad(JOY_1);
        pressed = joy & ~lastJoy;
        lastJoy = joy;

        if(inBoot)
        {
            updateBoot(pressed);
        }
        else
        {
            if(joy & BUTTON_C)
            {
                hyperFlash = TRUE;

                if(corruption_level < 250)
                    corruption_level += 2;
                else
                    corruption_level = 255;
            }
            else
            {
                hyperFlash = FALSE;

                if(corruption_level > 0)
                    corruption_level--;
            }

            if(pressed & BUTTON_UP)
            {
                if(animSpeed < 6)
                    animSpeed++;
            }

            if(pressed & BUTTON_DOWN)
            {
                if(animSpeed > 1)
                    animSpeed--;
            }

            if(pressed & BUTTON_B)
                paused = !paused;

            if(pressed & BUTTON_START)
                autoMode = !autoMode;

            if(pressed & BUTTON_A)
                nextScene();

            if(autoMode && !paused)
            {
                if(sceneTimer > SCENE_TIME)
                    nextScene();
            }

            updateScene();

            if(!paused)
            {
                animFrame += animSpeed;

                if(autoMode)
                    sceneTimer++;
            }
        }

        frame++;
        SYS_doVBlankProcess();
    }

    return 0;
}
