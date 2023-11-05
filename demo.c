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

    setTextCursor(0, 29);
    drawCharacterString("QXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSXSA");
    setFGColor(0,29, 0b11110000);

    while (1) {
        drawTextMode();
        shiftCharactersUp();
//        for (int i = 0; i < 40; i++) {
//            setFGColor(i, 0, rand() % 63);
//        }

        sleep_ms(5000);
    }

}

