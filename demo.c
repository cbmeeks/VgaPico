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

    int c = 0;
    for (int y = 0; y < 14; y++) {
        for (int x = 0; x < 20; x++) {
            drawCharacterAt(x, y, c);
            c++;
            if (c > 255)c = 0;
        }
    }


    while (1) {
        drawTextMode();
        toggleCursor();

//        drawLine(rand() % 160, rand() % 240, rand() % 160, rand() % 240, rand() % 64);

        sleep_ms(200);
    }

}

