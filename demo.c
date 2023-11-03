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

void drawChars();

int main() {
    initVGA();
    clearScreen();

    fillRect(0, 0, 320, 240, 0b11000011);

//    drawChars();

    clearTextMode(32);

    setTextCursor(1, 2);
    drawCharacterString("XX");

    while (1) {
        drawTextMode();
    }

}

void drawChars() {
    int c = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 40; x++) {
            draw8x8Char(x, y, c, 0b11001111, 0b11000011);
            c++;
            if (c > 255) return;
        }
    }
}
