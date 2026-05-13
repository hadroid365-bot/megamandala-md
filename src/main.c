#include <genesis.h>

#define TILE_VOID    TILE_USER_INDEX
#define TILE_DOT     (TILE_USER_INDEX + 1)
#define TILE_GRID    (TILE_USER_INDEX + 2)
#define TILE_LINE    (TILE_USER_INDEX + 3)
#define TILE_BAR     (TILE_USER_INDEX + 4)

#define SCENE_LEN 420
#define NUM_SCENES 4

static const u32 tile_void[8] =
{
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000,
    0x00000000
};

static const u32 tile_dot[8] =
{
    0x00077000,
    0x00777700,
    0x07777770,
    0x77777777,
    0x77777777,
    0x07777770,
    0x00777700,
    0x00077000
};

static const u32 tile_grid[8] =
{
    0x88888888,
    0x80000008,
    0x80099008,
    0x80099008,
    0x80000008,
    0x80099008,
    0x80099008,
    0x88888888
};

static const u32 tile_line[8] =
{
    0x33333333,
    0x00000000,
    0x44444444,
    0x00000000,
    0x55555555,
    0x00000000,
    0x66666666,
    0x00000000
};

static const u32 tile_bar[8] =
{
    0x0A0A0A0A,
    0x0A0A0A0A,
    0xB0B0B0B0,
    0xB0B0B0B0,
    0x0C0C0C0C,
    0x0C0C0C0C,
    0xD0D0D0D0,
    0xD0D0D0D0
};

static const u16 palCyber[16] =
{
    0x0000, 0x000E, 0x002E, 0x004E,
    0x006E, 0x008E, 0x00AE, 0x00CE,
    0x00EE, 0x02EE, 0x04EE, 0x06EE,
    0x08EE, 0x0AEE, 0x0CEE, 0x0EEE
};

static const u16 palFire[16] =
{
    0x0000, 0x0002, 0x0004, 0x0006,
    0x0008, 0x000A, 0x000C, 0x000E,
    0x020E, 0x040E, 0x060E, 0x080E,
    0x0A0E, 0x0C0E, 0x0E0E, 0x0EEE
};

static const u16 palVoid[16] =
{
    0x0000, 0x0222, 0x0444, 0x0666,
    0x0888, 0x0AAA, 0x0CCC, 0x0EEE,
    0x0CCC, 0x0AAA, 0x0888, 0x0666,
    0x0444, 0x0222, 0x000E, 0x00E0
};

static u16 pal0[16];
static u16 pal1[16];
static u8 lastScene = 255;

static u16 pickColor(u8 scene, u16 idx)
{
    if (scene == 1) return palFire[idx & 15];
    if (scene == 3) return palVoid[idx & 15];

    return palCyber[idx & 15];
}

static void updatePalettes(u16 frame, u8 scene)
{
    u16 i;

    pal0[0] = 0x0000;
    pal1[0] = 0x0000;

    for (i = 1; i < 16; i++)
    {
        pal0[i] = pickColor(scene, i + (frame >> 3));
        pal1[i] = pickColor(scene, 15 - i + (frame >> 4));
    }

    PAL_setPalette(PAL0, pal0, CPU);
    PAL_setPalette(PAL1, pal1, CPU);
}

static void loadTiles(void)
{
    VDP_loadTileData(tile_void, TILE_VOID, 1, DMA);
    VDP_loadTileData(tile_dot, TILE_DOT, 1, DMA);
    VDP_loadTileData(tile_grid, TILE_GRID, 1, DMA);
    VDP_loadTileData(tile_line, TILE_LINE, 1, DMA);
    VDP_loadTileData(tile_bar, TILE_BAR, 1, DMA);
}

static void drawBackground(u8 scene)
{
    u16 x, y, tile;

    VDP_clearPlane(BG_B, FALSE);

    for (y = 0; y < 28; y++)
    {
        for (x = 0; x < 40; x++)
        {
            tile = TILE_VOID;

            if (scene == 0)
            {
                if ((x < 7) || (x > 32) || (y < 4) || (y > 23))
                    tile = (((x + y) & 3) == 0) ? TILE_GRID : TILE_LINE;
            }
            else if (scene == 1)
            {
                if (((x + y) & 3) == 0)
                    tile = TILE_GRID;

                if ((x > 10) && (x < 30) && ((y & 3) == 0))
                    tile = TILE_LINE;
            }
            else if (scene == 2)
            {
                tile = (((x ^ y) & 1) == 0) ? TILE_BAR : TILE_GRID;
            }
            else
            {
                if ((x == 5) || (x == 34) || (y == 5) || (y == 22))
                    tile = TILE_LINE;

                if (((x + y) & 15) == 0)
                    tile = TILE_DOT;
            }

            VDP_setTileMapXY(
                BG_B,
                TILE_ATTR_FULL(PAL0, 0, 0, 0, tile),
                x,
                y
            );
        }
    }
}

static void putDot(u16 x, u16 y)
{
    VDP_setTileMapXY(
        BG_A,
        TILE_ATTR_FULL(PAL1, 1, 0, 0, TILE_DOT),
        x,
        y
    );
}

static void drawMandala(u16 frame)
{
    const u16 cx = 20;
    const u16 cy = 14;
    u16 r;
    u16 pulse = (frame >> 4) & 3;

    for (r = 0; r < 10; r++)
    {
        u16 rr = r + pulse;

        if (rr < 11)
        {
            putDot(cx + rr, cy);
            putDot(cx - rr, cy);
            putDot(cx, cy + rr);
            putDot(cx, cy - rr);
        }
    }

    for (r = 0; r < 7; r++)
    {
        u16 w = ((frame >> 4) + r) & 7;

        putDot(cx + w, cy + r);
        putDot(cx - w, cy - r);
        putDot(cx + r, cy - w);
        putDot(cx - r, cy + w);
    }
}

static void drawTunnel(u16 frame)
{
    const u16 cx = 20;
    const u16 cy = 14;
    u16 i;
    u16 phase = (frame >> 3) & 3;

    for (i = 0; i < 12; i++)
    {
        u16 left = 8 + i;
        u16 right = 32 - i;
        u16 top = 3 + i;
        u16 bottom = 25 - i;

        if (((i + phase) & 1) == 0)
        {
            putDot(left, top);
            putDot(right, top);
            putDot(left, bottom);
            putDot(right, bottom);
        }
    }

    for (i = 0; i < 10; i++)
    {
        putDot(cx - i, cy);
        putDot(cx + i, cy);
        putDot(cx, cy - i);
        putDot(cx, cy + i);
    }
}

static void drawGlitch(u16 frame)
{
    u16 i;
    u16 seed = frame >> 2;

    for (i = 0; i < 32; i++)
    {
        u16 x = (seed + (i * 7)) % 40;
        u16 y = ((seed >> 1) + (i * 5)) % 28;

        VDP_setTileMapXY(
            BG_A,
            TILE_ATTR_FULL(PAL1, 1, i & 1, (i >> 1) & 1, TILE_DOT),
            x,
            y
        );
    }

    VDP_drawText("NAO E BUG", 15, 4);
    VDP_drawText("E ORACULO", 15, 23);
}

static void drawVoid(u16 frame)
{
    const u16 cx = 20;
    const u16 cy = 14;
    u16 i;
    u16 open = (frame >> 5) & 7;

    for (i = 0; i < 8; i++)
    {
        putDot(cx - open - i, cy - i);
        putDot(cx + open + i, cy + i);
        putDot(cx - i, cy + open + i);
        putDot(cx + i, cy - open - i);
    }

    VDP_drawText("RESPIRA NO PIXEL", 11, 2);
    VDP_drawText("O SINAL ESTA VIVO", 11, 25);
}

static void drawScene(u16 frame, u8 scene)
{
    VDP_clearPlane(BG_A, FALSE);

    if (scene == 0)
    {
        drawMandala(frame);
        VDP_drawText("MEGAMANDALA MD", 12, 2);
        VDP_drawText("O CARTUCHO ESTA SONHANDO", 7, 25);
    }
    else if (scene == 1)
    {
        drawTunnel(frame);
        VDP_drawText("TUNEL DE SILICIO", 11, 2);
        VDP_drawText("NAO HA FASE", 13, 24);
        VDP_drawText("SO FREQUENCIA", 12, 25);
    }
    else if (scene == 2)
    {
        drawMandala(frame);
        drawGlitch(frame);
    }
    else
    {
        drawVoid(frame);
    }
}int main(bool hard)
{
    u16 frame = 0;

    SYS_disableInts();
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32, TRUE);
    SYS_enableInts();

    loadTiles();
    updatePalettes(0, 0);

    while(TRUE)
    {
        u8 scene = (frame / SCENE_LEN) % NUM_SCENES;

        if (scene != lastScene)
        {
            drawBackground(scene);
            lastScene = scene;
        }

        updatePalettes(frame, scene);

        if (scene == 0)
        {
            VDP_setHorizontalScroll(BG_B, frame >> 2);
            VDP_setVerticalScroll(BG_B, frame >> 4);
        }
        else if (scene == 1)
        {
            VDP_setHorizontalScroll(BG_B, frame);
            VDP_setVerticalScroll(BG_B, frame >> 2);
        }
        else if (scene == 2)
        {
            VDP_setHorizontalScroll(
                BG_B,
                (frame << 1) + (((frame & 31) == 0) ? 32 : 0)
            );

            VDP_setVerticalScroll(BG_B, frame >> 1);
        }
        else
        {
            VDP_setHorizontalScroll(BG_B, -(s16)(frame >> 3));
            VDP_setVerticalScroll(BG_B, 0);
        }

        drawScene(frame, scene);

        frame++;
        VDP_waitVSync();
    }

    return 0;
}
