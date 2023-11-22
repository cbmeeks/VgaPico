/**
 * Hunter Adams (vha3@cornell.edu)
 *
 *
 * HARDWARE CONNECTIONS
 *  - GPIO 8 ---> VGA Hsync
 *  - GPIO 9 ---> VGA Vsync

 *  - RP2040 GND ---> VGA GND
 *
 * RESOURCES USED
 *  - PIO state machines 0, 1, and 2 on PIO instance 0
 *  - DMA channels 0, 1, 2, and 3
 *  - 153.6 kBytes of RAM (for pixel color data)
 *
 * NOTE
 *  - This is a translation of the display primitives
 *    for the PIC32 written by Bruce Land and students
 *
 */


// GPIO pin configurations.
// For 6-bit color (64 colors), the RGB_PIN needs to have 5 other pins next to it (don't spread them out).
// Meaning, that is RGB_PIN is 2, then make sure you reserve 2, 3, 4, 5, 6 and 7.  These are GPIO pins.
enum vga_pins {
    HSYNC = 8, VSYNC = 9, RGB_PIN = 2
};

static unsigned char ascii2petscii[128] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x14, 0x09, 0x0d, 0x11, 0x93, 0x0a, 0x0e, 0x0f,
        0x10, 0x0b, 0x12, 0x13, 0x08, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
        0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
        0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
        0x40, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
        0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
        0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
        0xd8, 0xd9, 0xda, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
        0xc0, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
        0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
        0x58, 0x59, 0x5a, 0xdb, 0xdd, 0xdd, 0x5e, 0xdf,
};

enum colors {
    DARK_BLUE = 0b00000001,
    BLUE = 0b00000010,
    LIGHT_BLUE = 0b00000011,

    DARK_RED = 0b00010000,
    RED = 0b00100000,
    LIGHT_RED = 0b00110000,

    WHITE = 0b00111111,
};

#define swap(a, b) { short t = a; a = b; b = t; }

extern unsigned char vga_data_array[];
extern volatile uint32_t currentFrame;  // frame counter


void initVGA(void);

void dmaHandler();

void initDma(uint rgb_sm);

void initVGA(void);

void clearScreen(void);

void drawPixel(unsigned short x, unsigned short y, char color);

void drawVLine(short x, short y, short h, char color);

void drawHLine(short x, short y, short w, char color);

void drawLine(short x0, short y0, short x1, short y1, char color);

void drawRect(short x, short y, short w, short h, char color);

void drawRectCenter(short x, short y, short w, short h, char color);

void drawCircle(short x0, short y0, short r, char color);

void drawCircleHelper(short x0, short y0, short r, unsigned char cornername, char color);

void fillCircle(short x0, short y0, short r, char color);

void fillCircleHelper(short x0, short y0, short r, unsigned char cornername, short delta, char color);

void fillRect(short x, short y, short w, short h, char color);

void drawChar(short x, short y, unsigned char c, char color, char bg, unsigned char size);

unsigned char fontBuffer(unsigned short charIndex, unsigned charDataIndex);

void setFontBuffer(unsigned short charIndex, unsigned char charDataIndex, unsigned char data);

void clearTextMode(unsigned short charidx);


void draw8x8Char(unsigned short colx,
                 unsigned short coly,
                 unsigned short charidx,
                 unsigned char fgcolor,
                 unsigned char bgcolor);

void drawCharacterAt(unsigned short colx, unsigned short coly, unsigned short charidx);

void drawCharacter(unsigned short charidx, unsigned char fgcolor, unsigned char bgcolor);

void drawTextMode();

void setTextCursor(unsigned short x, unsigned short y);

void toggleCursor();

void drawCharacterString(char *str);

void _text_write(unsigned char c);

void clearFGColors(unsigned char color);

void clearBGColors(unsigned char color);

void setFGColor(unsigned short colx, unsigned short coly, unsigned char color);

void setBGColor(unsigned short colx, unsigned short coly, unsigned char color);

void shiftCharactersUp();

// Sprites
void draw16x16Sprite(unsigned char spriteNumber, int x, int y);

void drawToSpriteBuffer(unsigned char spriteNumber, unsigned short x, unsigned short y, unsigned char color);