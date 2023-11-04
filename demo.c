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

void drawChars2();

void drawChars3();

int main() {
    initVGA();
    clearScreen();

    clearTextMode(32);
    clearFGColors(0b11111100);
    clearBGColors(0b11000011);

    setTextCursor(0, 15);
//    drawCharacterString("SXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSX");

//    drawChars();
    drawChars2();
//    drawChars3();

    while (1) {
        drawTextMode();
//        shiftCharactersUp();

        for (int x = 0; x < 40; x++) {
            drawCharacter(x, 29 , rand() % 256);
        }

        sleep_ms(500);
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

void drawChars2() {
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 40; x++) {
            setFGColor(x, y, rand() % 64);
            setBGColor(x, y, rand() % 64);
            drawCharacter(x, y, rand() % 256);
        }
    }
}

void drawChars3() {
    for (int y = 0; y < 30; y++) {
        for (int x = 0; x < 40; x++) {
            drawCharacter(x, y, rand() % 256);
        }
    }
}