/*

*/


// Pico hardware includes
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"

// Generated PIO program includes
#include "hsync.pio.h"
#include "vsync.pio.h"
#include "rgb.pio.h"
#include "fonts/glcdfont.h"
//#include "font.h"

// Library includes
#include "vga_graphics.h"
#include "bit_helper.h"
#include "fonts/petscii/petscii.h"

// VGA defines
#define H_ACTIVE   655    // (active + front porch - 1) - cycle delay for MOV
#define V_ACTIVE   479    // (active - 1)
#define RGB_ACTIVE 319    // (horizontal active) / 2 - 1

// DMA channels
#define RGB_CHAN_0 0
#define RGB_CHAN_1 1

// Choose which PIO instance to use (there are two instances, each with 4 state machines)
PIO pio = pio0;

// Screen width / height
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
volatile uint32_t currentFrame;         // frame counter
volatile int currentScanLine = 0;       // current processed scan line

#define TXCOUNT (SCREEN_WIDTH * SCREEN_HEIGHT)
#define DMATXCOUNT SCREEN_WIDTH
unsigned char vga_data_array[TXCOUNT];
volatile unsigned char *address_pointer_array = &vga_data_array[0];

// The 40x30 character buffer
#define TEXT_MODE_WIDTH 40
#define TEXT_MODE_HEIGHT 30
#define TEXT_MODE_COUNT (TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT)
unsigned char text_buffer[TEXT_MODE_COUNT];
unsigned char text_fg_color_buffer[TEXT_MODE_COUNT];
unsigned char text_bg_color_buffer[TEXT_MODE_COUNT];

// Text Mode cursor
unsigned short cursor_x, cursor_y;

// For drawLine
#define swap(a, b) \
  { \
    short t = a; \
    a = b; \
    b = t; \
  }

//////////////////////////////////////////////////////////////////////
// For drawing characters (original...will be removed)
// For writing text
#define tabspace 4  // number of spaces for a tab
// For accessing the font library
#define pgm_read_byte(addr) (*(const unsigned char*)(addr))
unsigned short textsize;
char textcolor, textbgcolor, wrap;
//////////////////////////////////////////////////////////////////////


void initVGA() {
    // Set PIO program offset
    uint hsync_offset = pio_add_program(pio, &hsync_program);
    uint vsync_offset = pio_add_program(pio, &vsync_program);
    uint rgb_offset = pio_add_program(pio, &rgb_program);

    // Manually set SM numbers for PIO0
    uint hsync_sm = 0;
    uint vsync_sm = 1;
    uint rgb_sm = 2;

    // Call the initialization functions that are defined within each PIO file.
    hsync_program_init(pio, hsync_sm, hsync_offset, HSYNC);
    vsync_program_init(pio, vsync_sm, vsync_offset, VSYNC);
    rgb_program_init(pio, rgb_sm, rgb_offset, RGB_PIN);

    // Initialize PIO state machine counters. This passes the information to the state machines
    // that they retrieve in the first 'pull' instructions, before the .wrap_target directive
    // in the assembly. Each uses these values to initialize some counting registers.
    pio_sm_put_blocking(pio, hsync_sm, H_ACTIVE);
    pio_sm_put_blocking(pio, vsync_sm, V_ACTIVE);
    pio_sm_put_blocking(pio, rgb_sm, RGB_ACTIVE);

    // Start the state machines in sync
    pio_enable_sm_mask_in_sync(pio, ((1u << hsync_sm) | (1u << vsync_sm) | (1u << rgb_sm)));

    // Initialize and start DMA
    initDma(rgb_sm);
}

void initDma(uint rgb_sm) {

    // Channel Zero (sends color data to PIO VGA machine)
    dma_channel_config c0 = dma_channel_get_default_config(RGB_CHAN_0);  // default configs
    channel_config_set_transfer_data_size(&c0, DMA_SIZE_8);              // 8-bit transfers
    channel_config_set_read_increment(&c0, true);                        // yes read incrementing
    channel_config_set_write_increment(&c0, false);                      // no write incrementing
    channel_config_set_dreq(&c0, DREQ_PIO0_TX2);                         // DREQ_PIO0_TX2 pacing (FIFO)
    channel_config_set_chain_to(&c0, RGB_CHAN_1);                        // chain to other channel

    dma_channel_configure(
            RGB_CHAN_0,                 // Channel to be configured
            &c0,                        // The configuration we just created
            &pio->txf[rgb_sm],          // write address (RGB PIO TX FIFO)
            &vga_data_array,            // The initial read address (pixel color array)
            DMATXCOUNT,                 // Number of transfers; in this case each is 1 byte.
            false                       // Don't start immediately.
    );

    // Channel One (reconfigures the first channel)
    dma_channel_config c1 = dma_channel_get_default_config(RGB_CHAN_1);   // default configs
    channel_config_set_transfer_data_size(&c1, DMA_SIZE_32);              // 32-bit transfers
    channel_config_set_read_increment(&c1, false);                        // no read incrementing
    channel_config_set_write_increment(&c1, false);                       // no write incrementing
    channel_config_set_chain_to(&c1, RGB_CHAN_0);                         // chain to other channel

    dma_channel_configure(
            RGB_CHAN_1,                         // Channel to be configured
            &c1,                                // The configuration we just created
            &dma_hw->ch[RGB_CHAN_0].read_addr,  // Write address (channel 0 read address)
            &address_pointer_array,             // Read address (POINTER TO AN ADDRESS)
            1,                                  // Number of transfers, in this case each is 4 byte
            false                               // Don't start immediately.
    );

    // enable DMA channel IRQ0
    dma_channel_set_irq0_enabled(RGB_CHAN_0, true);

    // set DMA IRQ handler
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // set highest IRQ priority
    irq_set_priority(DMA_IRQ_0, 0);

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continuously DMA'd to the PIO machines that are driving the screen.
    // To change the contents of the screen, we need only change the contents
    // of that array.
    dma_start_channel_mask((1u << RGB_CHAN_0));
}

// DMA handler - called at the end of every scanline
void dma_handler() {

    // Clear the interrupt request for DMA control channel
    dma_hw->ints0 = (1u << RGB_CHAN_0);

    // increment scanline (1..)
    currentScanLine++;                  // new current scanline
    if (currentScanLine >= 480) {       // last scanline?
        currentScanLine = 0;            // restart scanline
        currentFrame++;                 // increment frame counter
    }

    address_pointer_array = &vga_data_array[DMATXCOUNT * ((currentScanLine + 0) >> 1)];
}


// A function for drawing a pixel with a specified color.
// Note that because information is passed to the PIO state machines through
// a DMA channel, we only need to modify the contents of the array and the
// pixels will be automatically updated on the screen.
void drawPixel(short x, short y, char color) {
    // Range checks (320x240 display)
    if ((x < SCREEN_WIDTH - 1) && (x >= 0) && (y >= 0) && (y < SCREEN_HEIGHT - 1)) {
        int pixel = ((SCREEN_WIDTH * y) + x);
        vga_data_array[pixel] = color;
    }
}

void clearScreen() {
    for (int i = 0; i < TXCOUNT; i++) {
        vga_data_array[i] = 0;
    }
}

void drawVLine(short x, short y, short h, char color) {
    for (short i = y; i < (y + h); i++) {
        drawPixel(x, i, color);
    }
}

void drawHLine(short x, short y, short w, char color) {
    for (short i = x; i < (x + w); i++) {
        drawPixel(i, y, color);
    }
}

// Bresenham's algorithm - thx wikipedia and thx Bruce!
void drawLine(short x0, short y0, short x1, short y1, char color) {
    /* Draw a straight line from (x0,y0) to (x1,y1) with given color
      Parameters:
           x0: x-coordinate of starting point of line. The x-coordinate of
               the top-left of the screen is 0. It increases to the right.
           y0: y-coordinate of starting point of line. The y-coordinate of
               the top-left of the screen is 0. It increases to the bottom.
           x1: x-coordinate of ending point of line. The x-coordinate of
               the top-left of the screen is 0. It increases to the right.
           y1: y-coordinate of ending point of line. The y-coordinate of
               the top-left of the screen is 0. It increases to the bottom.
           color: 3-bit color value for line
    */
    short steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        swap(x0, y0);
        swap(x1, y1);
    }

    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }

    short dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    short err = dx / 2;
    short ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0 <= x1; x0++) {
        if (steep) {
            drawPixel(y0, x0, color);
        } else {
            drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

// Draw a rectangle
void drawRect(short x, short y, short w, short h, char color) {
    /* Draw a rectangle outline with top left vertex (x,y), width w
      and height h at given color
      Parameters:
           x:  x-coordinate of top-left vertex. The x-coordinate of
               the top-left of the screen is 0. It increases to the right.
           y:  y-coordinate of top-left vertex. The y-coordinate of
               the top-left of the screen is 0. It increases to the bottom.
           w:  width of the rectangle
           h:  height of the rectangle
           color:  16-bit color of the rectangle outline
      Returns: Nothing
    */
    drawHLine(x, y, w, color);
    drawHLine(x, y + h - 1, w, color);
    drawVLine(x, y, h, color);
    drawVLine(x + w - 1, y, h, color);
}

void drawRectCenter(short x, short y, short w, short h, char color) {
    drawRect(x - w / 2, y - h / 2, w, h, color);
}

void drawCircle(short x0, short y0, short r, char color) {
    /* Draw a circle outline with center (x0,y0) and radius r, with given color
      Parameters:
           x0: x-coordinate of center of circle. The top-left of the screen
               has x-coordinate 0 and increases to the right
           y0: y-coordinate of center of circle. The top-left of the screen
               has y-coordinate 0 and increases to the bottom
           r:  radius of circle
           color: 16-bit color value for the circle. Note that the circle
               isn't filled. So, this is the color of the outline of the circle
      Returns: Nothing
    */
    short f = 1 - r;
    short ddF_x = 1;
    short ddF_y = -2 * r;
    short x = 0;
    short y = r;

    drawPixel(x0, y0 + r, color);
    drawPixel(x0, y0 - r, color);
    drawPixel(x0 + r, y0, color);
    drawPixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        drawPixel(x0 + x, y0 + y, color);
        drawPixel(x0 - x, y0 + y, color);
        drawPixel(x0 + x, y0 - y, color);
        drawPixel(x0 - x, y0 - y, color);
        drawPixel(x0 + y, y0 + x, color);
        drawPixel(x0 - y, y0 + x, color);
        drawPixel(x0 + y, y0 - x, color);
        drawPixel(x0 - y, y0 - x, color);
    }
}

void drawCircleHelper(short x0, short y0, short r, unsigned char cornername, char color) {
    // Helper function for drawing circles and circular objects
    short f = 1 - r;
    short ddF_x = 1;
    short ddF_y = -2 * r;
    short x = 0;
    short y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            drawPixel(x0 + x, y0 + y, color);
            drawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            drawPixel(x0 - y, y0 + x, color);
            drawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 - x, y0 - y, color);
        }
    }
}

void fillCircle(short x0, short y0, short r, char color) {
    /* Draw a filled circle with center (x0,y0) and radius r, with given color
      Parameters:
           x0: x-coordinate of center of circle. The top-left of the screen
               has x-coordinate 0 and increases to the right
           y0: y-coordinate of center of circle. The top-left of the screen
               has y-coordinate 0 and increases to the bottom
           r:  radius of circle
           color: 16-bit color value for the circle
      Returns: Nothing
    */
    drawVLine(x0, y0 - r, 2 * r + 1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

void fillCircleHelper(short x0, short y0, short r, unsigned char cornername, short delta, char color) {
    // Helper function for drawing filled circles
    short f = 1 - r;
    short ddF_x = 1;
    short ddF_y = -2 * r;
    short x = 0;
    short y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        if (cornername & 0x1) {
            drawVLine(x0 + x, y0 - y, 2 * y + 1 + delta, color);
            drawVLine(x0 + y, y0 - x, 2 * x + 1 + delta, color);
        }
        if (cornername & 0x2) {
            drawVLine(x0 - x, y0 - y, 2 * y + 1 + delta, color);
            drawVLine(x0 - y, y0 - x, 2 * x + 1 + delta, color);
        }
    }
}

// fill a rectangle
void fillRect(short x, short y, short w, short h, char color) {
    /* Draw a filled rectangle with starting top-left vertex (x,y),
       width w and height h with given color
      Parameters:
           x:  x-coordinate of top-left vertex; top left of screen is x=0
                   and x increases to the right
           y:  y-coordinate of top-left vertex; top left of screen is y=0
                   and y increases to the bottom
           w:  width of rectangle
           h:  height of rectangle
           color:  3-bit color value
      Returns:     Nothing
    */

    for (int i = x; i < (x + w); i++) {
        for (int j = y; j < (y + h); j++) {
            drawPixel(i, j, color);
        }
    }
}

/**
 * Draw an 8x8 character using the PETSCII font to the screen at 40x30 characters
 * @param colx Screen column X
 * @param coly Screen column Y
 * @param charidx Character index from PETSCII font buffer
 * @param fgcolor Foreground color
 * @param bgcolor Background color
 */
void draw8x8Char(unsigned short colx,
                 unsigned short coly,
                 unsigned short charidx,
                 unsigned char fgcolor,
                 unsigned char bgcolor) {
    for (int y = 0; y < 8; y++) {
        unsigned char line = petscii[charidx][y];

        // get the starting x/y pixel location
        short scrx = colx * 8;
        short scry = coly * 8;

        for (int x = 0; x < 8; x++) {
            drawPixel(scrx + x, scry + y, BitVal(line, (7 - x)) == 0x01 ? fgcolor : bgcolor);
        }
    }
}

/**
 * Draws the entire text mode character buffer.
  */
void drawTextMode() {
    int x = 0;
    int y = 0;
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        int index = y * TEXT_MODE_HEIGHT + x;
        draw8x8Char(x, y, text_buffer[index], text_fg_color_buffer[index], text_bg_color_buffer[index]);
        x++;
        if (x >= TEXT_MODE_WIDTH) {
            x = 0;
            y++;
            if (y >= TEXT_MODE_HEIGHT) y = 0;
        }
    }
}

/**
 * Draws a single PETSCII character (from PETSCII font buffer) to the screen buffer
 * @param colx Screen column X
 * @param coly Screen column Y
 * @param charidx Character index from within font buffer
 */
void drawCharacter(unsigned short colx, unsigned short coly, unsigned short charidx) {
    if (colx < 0 || colx >= TEXT_MODE_WIDTH) return;
    if (coly < 0 || coly >= TEXT_MODE_HEIGHT) return;
    if (charidx < 0 || charidx >= 256) return;
    text_buffer[coly * TEXT_MODE_HEIGHT + colx] = charidx;
}

void clearTextMode(unsigned short charidx) {
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        text_buffer[i] = charidx;
    }
}

void setTextCursor(unsigned short x, unsigned short y) {
    cursor_x = x;
    cursor_y = y;
}

void drawCharacterString(char *str) {
    while (*str) {
        _text_write(*str++);
    }
}

void _text_write(unsigned char c) {
    if (c == '\n') {

    } else if (c == '\r') {

    } else if (c == '\t') {

    } else {
        drawCharacter(cursor_x, cursor_y, c);
        cursor_x++;
        // TODO handle wrapping screen
    }
}


void clearFGColors(unsigned char color) {
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        text_fg_color_buffer[i] = color;
    }
}

void clearBGColors(unsigned char color) {
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        text_bg_color_buffer[i] = color;
    }
}

void setFGColor(unsigned short colx, unsigned short coly, unsigned char color) {
    if (colx < 0 || colx >= TEXT_MODE_WIDTH) return;
    if (coly < 0 || coly >= TEXT_MODE_HEIGHT) return;
    text_fg_color_buffer[coly * TEXT_MODE_HEIGHT + colx] = color;
}

void setBGColor(unsigned short colx, unsigned short coly, unsigned char color) {
    if (colx < 0 || colx >= TEXT_MODE_WIDTH) return;
    if (coly < 0 || coly >= TEXT_MODE_HEIGHT) return;
    text_bg_color_buffer[coly * TEXT_MODE_HEIGHT + colx] = color;
}

void shiftCharactersUp() {
    for (int y = 1; y < TEXT_MODE_HEIGHT; y++) {
        for (int x = 0; x < TEXT_MODE_WIDTH; x++) {
            text_buffer[((y - 1) * TEXT_MODE_HEIGHT) + x] = text_buffer[(y * TEXT_MODE_HEIGHT) + x];
            text_fg_color_buffer[((y - 1) * TEXT_MODE_HEIGHT) + x] = text_fg_color_buffer[(y * TEXT_MODE_HEIGHT) + x];
            text_bg_color_buffer[((y - 1) * TEXT_MODE_HEIGHT) + x] = text_bg_color_buffer[(y * TEXT_MODE_HEIGHT) + x];
        }
    }
}


///////////////////////////////////////////////////////////////////////////////////////
// OLD, original code
///////////////////////////////////////////////////////////////////////////////////////

// Draw a character
void drawChar(short x, short y, unsigned char c, char color, char bg, unsigned char size) {
    char i, j;
    if ((x >= SCREEN_WIDTH) ||          // Clip right
        (y >= SCREEN_HEIGHT) ||         // Clip bottom
        ((x + 8 * size - 1) < 0) ||     // Clip left
        ((y + 8 * size - 1) < 0))       // Clip top
        return;

    // this is set for 5x7 fonts. TODO create a true 8x8 version
    for (i = 0; i < 6; i++) {
        unsigned char line;
        if (i == 5)
            line = 0x0;
        else
            line = pgm_read_byte(font + (c * 5) + i);
        for (j = 0; j < 8; j++) {
            if (line & 0x1) {
                if (size == 1)  // default size
                    drawPixel(x + i, y + j, color);
                else {  // big size
                    fillRect(x + (i * size), y + (j * size), size, size, color);
                }
            } else if (bg != color) {
                if (size == 1)  // default size
                    drawPixel(x + i, y + j, bg);
                else {  // big size
                    fillRect(x + i * size, y + j * size, size, size, bg);
                }
            }
            line >>= 1;
        }
    }
}

void setTextSize(unsigned char s) {
    /*Set size of text to be displayed
      Parameters:
           s = text size (1 being smallest)
      Returns: nothing
    */
    textsize = (s > 0) ? s : 1;
}

void setTextColor(char c) {
    // For 'transparent' background, we'll set the bg
    // to the same as fg instead of using a flag
    textcolor = textbgcolor = c;
}

void setFgBgTextColor(char fg, char bg) {
    textcolor = fg;
    textbgcolor = bg;
}

void setTextWrap(char w) {
    wrap = w;
}

void tft_write(unsigned char c) {
    if (c == '\n') {
        cursor_y += textsize * 8;
        cursor_x = 0;
    } else if (c == '\r') {
        // skip em
    } else if (c == '\t') {
        int new_x = cursor_x + tabspace;
        if (new_x < SCREEN_WIDTH) {
            cursor_x = new_x;
        }
    } else {
        drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize);
        cursor_x += textsize * 8;
        if (wrap && (cursor_x > (SCREEN_WIDTH - textsize * 8))) {
            cursor_y += textsize * 8;
            cursor_x = 0;
        }
    }
}

void writeString(char *str) {
    /* Print text onto screen
      Call setTextCursor(), setTextColor(), setTextSize() as necessary before printing
    */
    while (*str) {
        tft_write(*str++);
    }
}