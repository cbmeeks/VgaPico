// Special thanks to:
// Hunter Adams (https://vanhunteradams.com/Pico/VGA/VGA.html) and
// https://github.com/Pancra85/VGA_graphics/blob/main/vga_graphics.cpp

// VgaPico by cbmeeks
// Code taken from other libraries and tweaked to support 320x240 resolution and 64 colors.
// Lots of bugs and issues.  Use at your own risk.
// The goal was to be something I could learn from while trying to keep it "simple" enough
// to fully understand it.

#include <stdlib.h>
#include "pico/stdlib.h"
#include "vga_graphics.h"

int main() {
    initVGA();
    clearScreen();

    clearTextMode(32);
    clearFGColors(0b11111111);
    clearBGColors(0b11000011);

    setTextCursor(0, 28);
    drawCharacterString("QXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSA");

    setTextCursor(0, 15);
    drawCharacterString("QXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSA");

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            drawToSpriteBuffer(0, x, y, 0b11111100);
        }
    }

    drawToSpriteBuffer(0, 4, 4, 0b11000000);
    drawToSpriteBuffer(0, 4, 5, 0b11000000);
    drawToSpriteBuffer(0, 4, 6, 0b11000000);
    drawToSpriteBuffer(0, 4, 7, 0b11000000);
    drawToSpriteBuffer(0, 4, 8, 0b11000000);
    drawToSpriteBuffer(0, 4, 9, 0b11000000);

    int x = 0;

    while (1) {
        drawTextMode();
        toggleCursor();

//        drawPixel(100, 100, 0b11111100);
        draw16x16Sprite(0, x, 116);
        x++;
        if (x > 320) x = 0;
//        shiftCharactersUp();
//        for (int i = 0; i < 40; i++) {
//            setFGColor(i, 0, rand() % 63);
//        }

//        sleep_ms(200);
    }

}

