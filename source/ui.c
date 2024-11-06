// SPDX-License-Identifier: MIT
//
// SPDX-FileContributor: Adrian "asie" Siekierka, 2024

#include <stdio.h>
#include <nds.h>
#include <nds/arm9/dldi.h>
#include <fat.h>
#include "ui.h"

// #include <font.h>
#include <topLogo.h>

static PrintConsole bottomConsole, topConsole;

extern uint32_t dcd_read_chip_id(void);

static int bg;

ITCM_CODE void ui_toggle_blink_activity(void) {
    topConsole.fontBgMap[(23 * 32) + 30] ^= 0xA000;
}

ITCM_CODE void ui_toggle_blink_write_activity(void) {
    topConsole.fontBgMap[(23 * 32) + 29] ^= 0x9000;
}


void ui_init(void) {
    videoSetMode(MODE_4_2D);
	videoSetModeSub(MODE_0_2D);
    vramSetPrimaryBanks(VRAM_A_LCD, VRAM_B_LCD, VRAM_C_SUB_BG, VRAM_D_MAIN_BG_0x06000000);
    // setBrightness(3, 0);
	
	bg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 1, 0);
	// Load graphics after font or else you get palette conflicts. :P
	decompress(topLogoBitmap, bgGetGfxPtr(bg), LZ77Vram);
	dmaCopy(topLogoPal, BG_PALETTE, 256*2);
	
	consoleInit(&bottomConsole,
        0, BgType_Text4bpp, BgSize_T_256x256, 22, 3, false, true);
    consoleInit(&topConsole,
        0, BgType_Text4bpp, BgSize_T_256x256, 4, 6, true, true); // Reconfigured to allow text and 8bit image.
		
	/*ConsoleFont font;
	font.gfx = (u16*)fontTiles;
	font.pal = NULL;
	font.numChars = 95;
	font.numColors =  0;
	font.bpp = 1;
	font.asciiOffset = 32;
	
	consoleSetFont(&bottomConsole, &font);
	consoleSetFont(&topConsole, &font);*/
		
	// BG_PALETTE[255] = RGB15(31,31,31);
	// BG_PALETTE_SUB[255] = RGB15(31,31,31);
	
    consoleSelect(&topConsole);

    /*puts("\x1b[2J"     
         "\x1b[4;0H"     
         "\x1b[37;1m"     
         "                _\n"     
         "     _ __  _ __(_) __\n"     
         "    | '_ \\| '__| |'_ \\\n"     
         "    | | | | |  | |(_) )\n"     
         "    |_| |_|_|  |_|.__/\n"     
         "\x1b[37;0m"     
         "     _   _   __| |__\n"     
         "    | | | | / _| '_ \\\n"     
         "    | |_| |_\\_ \\ |_) )\n"     
         "     \\____|____/____/\n"     
         "\x1b[30;1m"     
         "      __| (_)__| | __\n"     
         "     / _' | / _| |/ /\n"     
         "    ( (_| | \\_ \\   <  "     
         "\x1b[37;0m" VERSION     
         "\x1b[30;1m\n"     
         "     \\__,_|_|__/_|\\_\\ " GIT_HASH
         "\x1b[21;0H");
    printf("\x1b[37;0m%s", io_dldi_data->friendlyName);*/
	
	for (int i = 0; i < 15; i++)printf("\n");	
	puts("                      " VERSION "\x1b[30;1m\n"
         "                      " GIT_HASH "\x1b[21;0H");
    printf("\x1b[37;0m%s", io_dldi_data->friendlyName);

    consoleSelect(&bottomConsole);
    puts("\x1b[2J" "\x1b[23;0H");

    for (int i = 0; i < 8; i++) {
        topConsole.fontBgMap[(23 * 32) + 24 + i] = ((uint8_t) '*') + topConsole.fontCharOffset - topConsole.font.asciiOffset;
    }
}

void ui_show_chip_id(void) {
    uint32_t chip_id = dcd_read_chip_id();

    consoleSelect(&topConsole);
    printf("\x1b[21;0H" "\x1b[37;1mUSB controller %04X, rev %02X", (int) ((chip_id >> 8) & 0xFFFF), (int) (chip_id & 0xFF));
    consoleSelect(&bottomConsole);
}

void ui_select_top(void) {
    consoleSelect(&topConsole);
}

void ui_select_bottom(void) {
    consoleSelect(&bottomConsole);
}

