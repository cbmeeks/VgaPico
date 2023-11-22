/*

*/

// Graphic mode defines
// NOTE!  These defines must match whatever PIO you are compiling in your CMakeLists.txt file.
// Horizontal values:   __HORIZONTAL_640__, __HORIZONTAL_320__, __HORIZONTAL_160__, __HORIZONTAL_80__, __HORIZONTAL_40__
// Vertical values:     __VERTICAL__240__, __VERTICAL__120__, __VERTICAL__60__, __VERTICAL__30__
#define __HORIZONTAL_320__
#define __VERTICAL__240__


// Pico hardware includes
#include <stdlib.h>
#include <pico/stdio.h>
#include <pico/printf.h>
#include "hardware/pio.h"
#include "hardware/dma.h"

// Generated PIO program includes
#include "pio/hsync.pio.h"
#include "pio/vsync.pio.h"
#include "pio/rgb.pio.h"

// Library includes
#include "vga_graphics.h"
#include "vga_base.h"
#include "bit_helper.h"
#include "fonts/petscii/petscii.h"

// VGA defines
#define H_ACTIVE   655    // (active + front porch - 1) - cycle delay for MOV
#define V_ACTIVE   479    // (active - 1)

#ifdef __HORIZONTAL_640__
#define RGB_ACTIVE 639    // (horizontal active) - 1
#endif

#ifdef __HORIZONTAL_320__
#define RGB_ACTIVE 319    // (horizontal active) / 2 - 1
#endif

#ifdef __HORIZONTAL_160__
#define RGB_ACTIVE 159    // (horizontal active) / 4 - 1
#endif

#ifdef __HORIZONTAL_80__
#define RGB_ACTIVE 79    // (horizontal active) / 8 - 1
#endif

#ifdef __HORIZONTAL_40__
#define RGB_ACTIVE 39    // (horizontal active) / 16 - 1
#endif

// DMA channels
#define RGB_CHAN_0 0
#define RGB_CHAN_1 1

// Choose which PIO instance to use (there are two instances, each with 4 state machines)
PIO pio = pio0;

// Screen width / height

#ifdef __HORIZONTAL_640__
#define SCREEN_WIDTH 640
#endif

#ifdef __HORIZONTAL_320__
#define SCREEN_WIDTH 320
#endif

#ifdef __HORIZONTAL_160__
#define SCREEN_WIDTH 160
#endif

#ifdef __HORIZONTAL_80__
#define SCREEN_WIDTH 80
#endif

#ifdef __HORIZONTAL_40__
#define SCREEN_WIDTH 40
#endif

#ifdef __VERTICAL__240__
#define SCREEN_HEIGHT 240
#endif

#ifdef __VERTICAL__120__
#define SCREEN_HEIGHT 120
#endif

#ifdef __VERTICAL__60__
#define SCREEN_HEIGHT 60
#endif

#ifdef __VERTICAL__30__
#define SCREEN_HEIGHT 30
#endif


volatile uint32_t currentFrame;         // frame counter
volatile int currentScanLine = 0;       // current processed scan line


/**
 * VGA Data Array
 * This array is a "one pixel per byte" buffer that holds all pixels in a 320x240 resolution (or whatever the resolution is).
 * Each byte in this array corresponds to one pixel in the following format:  XXRRGGBB.
 * The top two bits of the byte are currently ignored.  Meaning we have six bits of color that
 * results in 63 colors plus black.
 *
 * For example, white would be built as:  XX111111   where "X" is an ignored bit.
 * Bright green = XX001100
 * Bright blue = XX000011
 * etc.
 */
#define TXCOUNT (SCREEN_WIDTH * SCREEN_HEIGHT)
#define DMATXCOUNT SCREEN_WIDTH

// The pixel array
unsigned char vga_data_array[TXCOUNT];
volatile unsigned char *vga_data_array_ptr = &vga_data_array[0];

// The character buffer

#ifdef __HORIZONTAL_640__
#define TEXT_MODE_WIDTH 80
#endif

#ifdef __HORIZONTAL_320__
#define TEXT_MODE_WIDTH 40
#endif

#ifdef __HORIZONTAL_160__
#define TEXT_MODE_WIDTH 20
#endif

#ifdef __HORIZONTAL_80__
#define TEXT_MODE_WIDTH 10
#endif

#ifdef __HORIZONTAL_40__
#define TEXT_MODE_WIDTH 5
#endif

#ifdef __VERTICAL__240__
#define TEXT_MODE_HEIGHT 30
#endif

#ifdef __VERTICAL__120__
#define TEXT_MODE_HEIGHT 15
#endif

#ifdef __VERTICAL__60__
#define TEXT_MODE_HEIGHT 7
#endif

#ifdef __VERTICAL__30__
#define TEXT_MODE_HEIGHT 3
#endif


#define TEXT_MODE_COUNT (TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT)

/**
 * TEXT BUFFER
 * Character buffer used to hold text and other characters.
 * Similar to the $0400 location on a Commodore 64.  Only 1200 bytes vs 1000.
 */
unsigned char text_buffer[TEXT_MODE_COUNT];

/**
 * FOREGROUND COLOR BUFFER
 * BACKGROUND COLOR BUFFER
 * A buffer the same size as the text_buffer so that each character has its own foreground and background color
 * from a palette of 64 colors.
 */
unsigned char text_fg_color_buffer[TEXT_MODE_COUNT];
unsigned char text_bg_color_buffer[TEXT_MODE_COUNT];

// Text Mode cursor position and colors
unsigned short cursor_x, cursor_y;
bool cursor_shown = true;   // whether this cursor image is current visible or the underlying text.

// Text Mode defaults
#define BLANK_CHAR 32
#define tabspace 4  // number of spaces for a tab

// Default colors
unsigned char foreground_color = WHITE;
unsigned char background_color = BLUE;


//////////////////////////////////////////////////////////////////////
// For drawing characters (original...will be removed)
// For writing text
unsigned short textsize;
char textcolor, textbgcolor, wrap;
//////////////////////////////////////////////////////////////////////


// Sprite buffer
unsigned char sprite_buffer[128][16 * 16] = {};


/**
 * Initialize the Pico for VGA video generation.
 * This method sets up the PIO (only one) and three state machines (SM) inside.
 * One SM is responsible for generating the HSYNC signal, one for the VSYNC signal
 * and one for RGB color generation.
 *
 * At the end of this method, we launch the DMA controller.
 */
void initVGA() {
    // TODO dynamically load the RGB PIO program into SM

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

/**
 * Initializes the DMA controller which is responsible for copying data into the RGB state machine
 * automatically.  This DMA controller (which uses two of the 12 channels) automatically sends data from
 * the {@see vga_data_array} to the RGB SM in the PIO.
 * @param rgb_sm the number of the RGM SM
 */
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
            &vga_data_array_ptr,             // Read address (POINTER TO AN ADDRESS)
            1,                                  // Number of transfers, in this case each is 4 byte
            false                               // Don't start immediately.
    );

    // enable DMA channel IRQ0
    dma_channel_set_irq0_enabled(RGB_CHAN_0, true);

    // set DMA IRQ handler
    irq_set_exclusive_handler(DMA_IRQ_0, dmaHandler);
    irq_set_enabled(DMA_IRQ_0, true);

    // set highest IRQ priority
    irq_set_priority(DMA_IRQ_0, 0);

    // Start DMA channel 0. Once started, the contents of the pixel color array
    // will be continuously DMA'd to the PIO machines that are driving the screen.
    // To change the contents of the screen, we need only change the contents
    // of that array.
    dma_start_channel_mask((1u << RGB_CHAN_0));
}

/**
 *  DMA Handler - called at the end of every scanline
 *  This callback needs to be VERY brief.  Do not put complex logic in here.
 *  The main purpose is to keep track the current scanline and frame.
 *  Also, it updates the starting address (the SOURCE) of the first DMA channel.
 *  This allows us to use "double-pixel" lines to get 240 resolution out of a 640x480 VGA signal.
 */
void dmaHandler() {
    // Clear the interrupt request for DMA control channel
    dma_hw->ints0 = (1u << RGB_CHAN_0);

    // increment scanline (1..)
    currentScanLine++;                  // new current scanline
    if (currentScanLine >= 480) {       // last scanline?
        currentScanLine = 0;            // restart scanline
        currentFrame++;                 // increment frame counter
    }

#ifdef __VERTICAL__240__
    vga_data_array_ptr = &vga_data_array[DMATXCOUNT * ((currentScanLine + 0) >> 1)];
#endif

#ifdef __VERTICAL__120__
    vga_data_array_ptr = &vga_data_array[DMATXCOUNT * ((currentScanLine + 0) >> 2)];
#endif

#ifdef __VERTICAL__60__
    vga_data_array_ptr = &vga_data_array[DMATXCOUNT * ((currentScanLine + 0) >> 3)];
#endif

#ifdef __VERTICAL__30__
    vga_data_array_ptr = &vga_data_array[DMATXCOUNT * ((currentScanLine + 0) >> 4)];
#endif
}


/**
 * Clears the screen down to the pixel level.
 * Defaults to 0b00000000
 */
void clearScreen() {
    for (int i = 0; i < TXCOUNT; i++) {
        vga_data_array[i] = 0b00000000;
    }
}

/**
 * A function for drawing a pixel with a specified color.
 * Note that because information is passed to the PIO state machines through a DMA channel,
 * we only need to modify the contents of the array and the pixels will be automatically updated on the screen.
 *
 * TODO check bounds checking (especially right side!)
 *
 * @param x X pixel location
 * @param y Y pixel location
 * @param color color to draw
 */
void drawPixel(unsigned short x, unsigned short y, char color) {
    if ((x < SCREEN_WIDTH - 1) && (x >= 0) && (y >= 0) && (y < SCREEN_HEIGHT - 1)) {
        int pixel = ((SCREEN_WIDTH * y) + x);
        vga_data_array[pixel] = color;
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
           color: 6-bit color value for line
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

void drawToSpriteBuffer(unsigned char spriteNumber, unsigned short x, unsigned short y, unsigned char color) {
    sprite_buffer[spriteNumber][(y * 16) + x] = color;
}

void draw16x16Sprite(unsigned char spriteNumber, int x, int y) {
    for (int line_y = 0; line_y < 16; line_y++) {
        for (int col_x = 0; col_x < 16; col_x++) {
            unsigned char color = (sprite_buffer[spriteNumber][(line_y * 16) + col_x]) & 0b00111111;
            if (color > 0)
                drawPixel(x + col_x, y + line_y, color);
        }
    }
}


/**
 * Draw an 8x8 character using the PETSCII font to the screen at 40x30 characters.
 * Note, that if the MSB of the color is set, the FG and BG colors will be flipped (inverse character).
 * For example, if you set the FG color to BLUE and the BG color to BLACK, then you will see a blue character on a black
 * background.  However, if you do the exact same thing but set bit 7, then you will see a BLACK character on a BLUE
 * background.
 *
 * Setting the MSB of the BG color has no effect.
 *
 * @param colx Screen column X
 * @param coly Screen column Y
 * @param charidx Character index from PETSCII font buffer
 * @param fgcolor Foreground color (0bIxRRGGBB: I = inverse, RR = red, GG = green, BB = blue)
 * @param bgcolor Background color
 */
void draw8x8Char(unsigned short colx,
                 unsigned short coly,
                 unsigned short charidx,
                 unsigned char fgcolor,
                 unsigned char bgcolor) {

    // get the starting x/y pixel location
    unsigned short scrx = (colx << 3);
    unsigned short scry = (coly << 3);

    if (CHECK_BIT(fgcolor, 7)) {
        for (int y = 0; y < 8; y++) {
            unsigned char line = petscii[charidx][y];
            drawPixel(scrx + 0, scry + y, CHECK_BIT(line, (7 - 0)) ? bgcolor : fgcolor);
            drawPixel(scrx + 1, scry + y, CHECK_BIT(line, (7 - 1)) ? bgcolor : fgcolor);
            drawPixel(scrx + 2, scry + y, CHECK_BIT(line, (7 - 2)) ? bgcolor : fgcolor);
            drawPixel(scrx + 3, scry + y, CHECK_BIT(line, (7 - 3)) ? bgcolor : fgcolor);
            drawPixel(scrx + 4, scry + y, CHECK_BIT(line, (7 - 4)) ? bgcolor : fgcolor);
            drawPixel(scrx + 5, scry + y, CHECK_BIT(line, (7 - 5)) ? bgcolor : fgcolor);
            drawPixel(scrx + 6, scry + y, CHECK_BIT(line, (7 - 6)) ? bgcolor : fgcolor);
            drawPixel(scrx + 7, scry + y, CHECK_BIT(line, (7 - 7)) ? bgcolor : fgcolor);
        }
    } else {
        for (int y = 0; y < 8; y++) {
            unsigned char line = petscii[charidx][y];
            drawPixel(scrx + 0, scry + y, CHECK_BIT(line, (7 - 0)) ? fgcolor : bgcolor);
            drawPixel(scrx + 1, scry + y, CHECK_BIT(line, (7 - 1)) ? fgcolor : bgcolor);
            drawPixel(scrx + 2, scry + y, CHECK_BIT(line, (7 - 2)) ? fgcolor : bgcolor);
            drawPixel(scrx + 3, scry + y, CHECK_BIT(line, (7 - 3)) ? fgcolor : bgcolor);
            drawPixel(scrx + 4, scry + y, CHECK_BIT(line, (7 - 4)) ? fgcolor : bgcolor);
            drawPixel(scrx + 5, scry + y, CHECK_BIT(line, (7 - 5)) ? fgcolor : bgcolor);
            drawPixel(scrx + 6, scry + y, CHECK_BIT(line, (7 - 6)) ? fgcolor : bgcolor);
            drawPixel(scrx + 7, scry + y, CHECK_BIT(line, (7 - 7)) ? fgcolor : bgcolor);
        }
    }
}

/**
 * Draws the entire text mode character buffer.
 * The TextMode is a 40x30* buffer that contains individual characters, each one byte.
 * The TextMode has a companion foreground and background color buffers as well.
 * Meaning, each character cell can have its own foreground and background color.
 * * The resolution is dynamic but 40x30 is the current max at 320x240 pixels
 */
void drawTextMode() {
    int x = 0;
    int y = 0;
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        int index = (y * TEXT_MODE_WIDTH) + x;
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
 * Draws a single PETSCII character at the current cursor position.
 * NOTE: This draws to the text buffer and the relative color buffers and not pixels directly.
 * NOTE: Cursor will advance down the screen but at the moment, the screen contents do not scroll up.
 * TODO add feature to scroll screen up when reaching the bottom-right
 * @param charidx character index from font buffer
 * @param fgcolor foreground color of character
 * @param bgcolor background color of character
 */
void drawCharacter(unsigned short charidx, unsigned char fgcolor, unsigned char bgcolor) {
    if (charidx < 0 || charidx > 255) return;

    text_buffer[(cursor_y * TEXT_MODE_WIDTH) + cursor_x] = charidx;
    text_fg_color_buffer[(cursor_y * TEXT_MODE_WIDTH) + cursor_x] = fgcolor;
    text_bg_color_buffer[(cursor_y * TEXT_MODE_WIDTH) + cursor_x] = bgcolor;

    cursor_x++;
    if (cursor_x >= TEXT_MODE_WIDTH) {
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= TEXT_MODE_HEIGHT) cursor_y = 0;
    }
}


/**
 * Draws a single PETSCII character (from PETSCII font buffer) to the screen buffer
 * NOTE: This draws to the text buffer and not pixels directly.
 * @param colx Screen column X
 * @param coly Screen column Y
 * @param charidx Character index from within font buffer
 */
void drawCharacterAt(unsigned short colx, unsigned short coly, unsigned short charidx) {
    if (colx < 0 || colx >= TEXT_MODE_WIDTH) return;
    if (coly < 0 || coly >= TEXT_MODE_HEIGHT) return;
    if (charidx < 0 || charidx > 255) return;
    text_buffer[(coly * TEXT_MODE_WIDTH) + colx] = charidx;
}

/**
 * Fill the text buffer with a chosen PETSCII character.
 *
 * @param charidx an index into the text_buffer {@see text_buffer}
 */
void clearTextMode(unsigned short charidx) {
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        text_buffer[i] = charidx;
    }
}


/**
 * Updates the screen cursor position based on character cells
 * @param x screen x column
 * @param y screen y row
 */
void setTextCursor(unsigned short x, unsigned short y) {
    cursor_x = x;
    cursor_y = y;
}

/**
 * Simply toggles the foreground and background colors at the current cursor position.
 * Note, this isn't exactly how the C64 does it.  The C64 will toggle between the current cursor color and the
 * current background color.  Look at the C64 for an example.  I might switch to this at some point but for now,
 * this works pretty well.
 */
void toggleCursor() {
    int index = (cursor_y * TEXT_MODE_WIDTH) + cursor_x;
    unsigned char fg = text_fg_color_buffer[index];
    unsigned char bg = text_bg_color_buffer[index];

    if (cursor_shown) {
        setFGColor(cursor_x, cursor_y, bg);
        setBGColor(cursor_x, cursor_y, fg);
    } else {
        setFGColor(cursor_x, cursor_y, fg);
        setBGColor(cursor_x, cursor_y, bg);
    }

    cursor_shown = !cursor_shown;
}


/**
 * Draws a string of characters to the text buffer.
 * Note, we are not currently handling wrapping.
 * Also, we're not really converting between ASCII and PETSCII properly.
 * We're still using a PETSCII system at the moment.
 * @param str string to write out
 */
void drawCharacterString(char *str) {
    while (*str) {
//        _text_write(*str++);
        _text_write(ascii2petscii[((*str++) & 0b00111111)]);
    }
}

void _text_write(unsigned char c) {

    if (c == '\n') {
        // newline pushes cursor down but not to the left
        cursor_y++;
        if (cursor_y >= TEXT_MODE_HEIGHT) {
            cursor_y = TEXT_MODE_HEIGHT - 1;
            shiftCharactersUp();
        }
    } else if (c == '\r') {
        // carriage return pushes cursor down and to the left 0 position
        cursor_x = 0;
        cursor_y++;
        if (cursor_y >= TEXT_MODE_HEIGHT) {
            cursor_y = TEXT_MODE_HEIGHT - 1;
            shiftCharactersUp();
        }
    } else if (c == '\t') {

    } else {
        drawCharacterAt(cursor_x, cursor_y, c);
        cursor_x++;
        if (cursor_x >= TEXT_MODE_WIDTH) {
            cursor_x = 0;
            cursor_y++;

            if (cursor_y >= TEXT_MODE_HEIGHT) {
                shiftCharactersUp();
                cursor_y = TEXT_MODE_HEIGHT - 1;
            }
        }
        // TODO handle wrapping screen
        // hint: reset cursor_x to 0 and use screen shift up if cursor_x > max x/y cursor position (bottom right)
    }
}


/**
 * Fills the foreground color buffer with a single color in XXRRGGBB format.
 * @param color the color
 */
void clearFGColors(unsigned char color) {
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        text_fg_color_buffer[i] = color;
    }
}

/**
 * Fills the background color buffer with a single color in XXRRGGBB format.
 * @param color the color
 */
void clearBGColors(unsigned char color) {
    for (int i = 0; i < TEXT_MODE_COUNT; i++) {
        text_bg_color_buffer[i] = color;
    }
}


/**
 * Sets the foreground color at a specified screen location (in 8x8 pixel cells)
 * @param colx screen x column
 * @param coly screen y column
 * @param color color
 */
void setFGColor(unsigned short colx, unsigned short coly, unsigned char color) {
    if (colx < 0 || colx >= TEXT_MODE_WIDTH) return;
    if (coly < 0 || coly >= TEXT_MODE_HEIGHT) return;
    text_fg_color_buffer[coly * TEXT_MODE_WIDTH + colx] = color;
}


/**
 * Sets the background color at a specified screen location (in 8x8 pixel cells)
 * @param colx screen x column
 * @param coly screen y column
 * @param color color
 */
void setBGColor(unsigned short colx, unsigned short coly, unsigned char color) {
    if (colx < 0 || colx >= TEXT_MODE_WIDTH) return;
    if (coly < 0 || coly >= TEXT_MODE_HEIGHT) return;
    text_bg_color_buffer[coly * TEXT_MODE_WIDTH + colx] = color;
}


/**
 * Shifts the entire text buffer screen up one line (40 chars).
 * Destroys the very top line and draws a blank line at the bottom.
 */
void shiftCharactersUp() {
    for (int y = 1; y < TEXT_MODE_HEIGHT; y++) {
        for (int x = 0; x < TEXT_MODE_WIDTH; x++) {
            text_buffer[((y - 1) * TEXT_MODE_WIDTH) + x] = text_buffer[(y * TEXT_MODE_WIDTH) + x];
            text_fg_color_buffer[((y - 1) * TEXT_MODE_WIDTH) + x] = text_fg_color_buffer[(y * TEXT_MODE_WIDTH) + x];
            text_bg_color_buffer[((y - 1) * TEXT_MODE_WIDTH) + x] = text_bg_color_buffer[(y * TEXT_MODE_WIDTH) + x];
        }
    }

    // now draw a blank line at the bottom with the current foreground/background color
    for (int i = 0; i < TEXT_MODE_WIDTH; i++) {
        setFGColor(i, TEXT_MODE_HEIGHT - 1, foreground_color);
        setBGColor(i, TEXT_MODE_HEIGHT - 1, background_color);
        drawCharacterAt(i, TEXT_MODE_HEIGHT - 1, BLANK_CHAR);
    }
}


unsigned char fontBuffer(unsigned short charIndex, unsigned charDataIndex) {
    // warning, this has no bounds checking!
    // TODO add bounds checking
    return petscii[charIndex][charDataIndex];
}

void setFontBuffer(unsigned short charIndex, unsigned char charDataIndex, unsigned char data) {
    // warning, this has no bounds checking!
    // TODO add bounds checking
    petscii[charIndex][charDataIndex] = data;
}


// OLD - Remove me
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
