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


// Give the I/O pins that we're using some names that make sense - usable in main()
enum vga_pins {
    HSYNC = 14, VSYNC = 15, RGB_PIN = 2
};

extern unsigned char vga_data_array[];
extern volatile uint32_t currentFrame;  // frame counter


void initVGA(void);
void dma_handler();
void initDma(uint rgb_sm);

// VGA primitives - usable in main
void initVGA(void) ;
void clearScreen(void);
void nextFrame(void);
void drawPixel(short x, short y, char color) ;
void drawVLine(short x, short y, short h, char color) ;
void drawHLine(short x, short y, short w, char color) ;
void drawLine(short x0, short y0, short x1, short y1, char color) ;
void drawRect(short x, short y, short w, short h, char color);
void drawRectCenter(short x, short y, short w, short h, char color);
void drawCircle(short x0, short y0, short r, char color) ;
void drawCircleHelper( short x0, short y0, short r, unsigned char cornername, char color) ;
void fillCircle(short x0, short y0, short r, char color) ;
void fillCircleHelper(short x0, short y0, short r, unsigned char cornername, short delta, char color) ;
void fillRect(short x, short y, short w, short h, char color) ;
void drawChar(short x, short y, unsigned char c, char color, char bg, unsigned char size) ;
void setTextCursor(short x, short y);
void setTextColor(char c);
void setFgBgTextColor(char fg, char bg);
void setTextSize(unsigned char s);
void setTextWrap(char w);
void tft_write(unsigned char c) ;
void writeString(char* str) ;

// Font methods
void clearTextMode(short charidx);
void draw8x8Char(short colx, short coly, short charidx, unsigned char fgcolor, unsigned char bgcolor);
void drawCharacter(short colx, short coly, short charidx);
void drawTextMode();
void drawCharacterString(char *str);
void _text_write(unsigned char c);