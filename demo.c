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

// Goofs (remove me)
#include "images/player.h"

int main() {
    initVGA();
    clearScreen();

    clearTextMode(32);
    clearFGColors(0b11111111);
    clearBGColors(0b11000100);
//
//    unsigned char c = 0;
//    int color = 1;
//    for (int y = 0; y < 15; y++) {
//        for (int x = 0; x < 20; x++) {
//            setFGColor(x, y, color % 64);
//            setBGColor(x, y, rand() % 64);
//            drawCharacterAt(x, y, c);
//            color++;
//            if (color > 63) color = 1;
//            c++;
//        }
//    }


    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            drawToSpriteBuffer(0, x, y, player[(y * 16) + x]);
        }
    }

    drawTextMode();
    while (1) {

        draw16x16Sprite(0, 40, 40);
        draw16x16Sprite(0, 70, 60);
        draw16x16Sprite(0, 16, 120 - 16);
//        toggleCursor();

//        shiftCharactersUp();

//        drawLine(rand() % 160, rand() % 120, rand() % 160, rand() % 120, rand() % 64);
//        fillCircle(rand() % 160, rand() % 120, rand() % 64, rand() % 64);

        sleep_ms(40);
    }

}

