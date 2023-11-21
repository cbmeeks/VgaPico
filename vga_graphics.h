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