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

    fillRect(0, 0, 320, 240, 0b11001011);
    fillRect(16, 16, 320 - 32, 240 - 32, 0b11000011);

    setTextColor(0b11001011);
    setTextSize(1);
    setTextCursor(4 * 8, 4 * 8);
    writeString("**** COMMODORE 64 BASIC V2 ****");

    while (1) {}
}

// Rename to main() to run this demo.
int Demo_02_main() {
    initVGA();
    clearScreen();

    int color = 0b00110000;

    for (int y = 32; y < 239; y++) {
        drawHLine(0, y, 319, color);
        color++;
        if (color > 63) color = 0;
    }

    fillCircle(160, 240 / 2, 100, 0b00110011);
    fillCircle(160, 240 / 2, 50, 0b00100001);
    fillCircle(160, 240 / 2, 20, 0b00010010);
    drawLine(0, 32, 319, 500, 0b11111111);

    setTextColor(0b11111111);
    setTextSize(1);
    setTextCursor(0, 1);
    writeString(" VgaPico-320x240 & 64 colors!  cbmeeks");
    setTextCursor(0, 9);
    writeString("123456789012345678901234567890123456789012345678901234567890");

    setTextColor(0b11001100);
    setTextCursor(1, 20);
    setTextSize(2);
    writeString("Large font support.");

    setTextColor(0b11111100);
    setTextCursor(1, 40);
    setTextSize(4);
    writeString("VIC-20?");

    setTextColor(0b11101000);
    setTextCursor(1, 80);
    setTextSize(6);
    writeString("Me big!");

    while (1) {}
}