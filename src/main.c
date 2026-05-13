#include <genesis.h>

#define TILE_CHECKER   TILE_USERINDEX
#define TILE_LINES     (TILE_USERINDEX + 1)
#define TILE_DOT       (TILE_USERINDEX + 2)

static const u32 tile_checker[8] =
{
    0x11112222,
    0x11112222,
    0x11112222,
    0x11112222,
    0x22221111,
    0x22221111,
    0x22221111,
    0x22221111
};

static const u32 tile_lines[8] =
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

static const u16 colorWheel[16] =
{
    0x0000, 0x000E, 0x002E, 0x004E,
    0x006E, 0x008E, 0x00AE, 0x00CE,
    0x00EE, 0x02EE, 0x04EE, 0x06EE,
    0x08EE, 0x0AEE, 0x0CEE, 0x0EEE
};

static u16 pal0[16];
static u16 pal1[16];

static void updatePalettes(u16 frame)
{
    u16 i;

    pal0[0] = 0x0000;
    pal1[0] = 0x0000;

    for (i = 1; i < 16; i++)
    {
        pal0[i] = colorWheel[(i + (frame >> 3)) & 15];
        pal1[i] = colorWheel[(15 - i + (frame >> 4)) & 15];
    }

    PAL_setPalette(PAL0, pal0, CPU);
    PAL_setPalette(PAL1, pal1, CPU);
}

static void drawBackground(void)
{
    u16 x, y;
    u16 tile;

    for (y = 0; y < 28; y++)
    {
        for (x = 0; x < 40; x++)
        {
            tile = (((x + y) & 1) == 0) ? TILE_CHECKER : TILE_LINES;
            VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, 0, 0, 0, tile), x, y);
        }
    }
}

static void drawMandala(u16 frame)
{
    const u16 cx = 20;
    const u16 cy = 14;
    u16 r;

    VDP_clearPlane(BG_A, FALSE);

    for (r = 0; r < 8; r++)
    {
        u16 offset = (frame >> 4) + r;
        u16 wobble = offset & 7;

        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 1, 0, 0, TILE_DOT), cx + r, cy);
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 1, 0, 1, TILE_DOT), cx - r, cy);
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 1, 1, 0, TILE_DOT), cx, cy + r);
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 1, 1, 1, TILE_DOT), cx, cy - r);

        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 1, 0, 0, TILE_DOT), cx + wobble, cy + r);
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL1, 1, 0, 1, TILE_DOT), cx - wobble, cy - r);
    }

    if ((frame & 127) < 64)
    {
        VDP_drawText("MEGAMANDALA MD", 12, 2);
        VDP_drawText("NAO HA FASE", 13, 24);
        VDP_drawText("SO FREQUENCIA", 12, 25);
    }
    else
    {
        VDP_drawText("HADRONIC DREAM ENGINE", 9, 2);
        VDP_drawText("O CARTUCHO ESTA SONHANDO", 7, 24);
    }
}

int main(bool hard)
{
    u16 frame = 0;

    SYS_disableInts();
    VDP_setScreenWidth320();
    VDP_setPlaneSize(64, 32, TRUE);
    SYS_enableInts();

    VDP_loadTileData(tile_checker, TILE_CHECKER, 1, DMA);
    VDP_loadTileData(tile_lines, TILE_LINES, 1, DMA);
    VDP_loadTileData(tile_dot, TILE_DOT, 1, DMA);

    updatePalettes(0);
    drawBackground();

    while(TRUE)
    {
        frame++;

        updatePalettes(frame);
        VDP_setHorizontalScroll(BG_B, frame >> 1);
        VDP_setVerticalScroll(BG_B, frame >> 3);
        drawMandala(frame);
        VDP_waitVSync();
    }

    return 0;
}
