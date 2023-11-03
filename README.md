# VgaPico
This is a VGA library for the Raspberry Pi Pico. It is based on the Hunter Adams code.

The intention is to be easy to learn and provide 320x240 resolution with 64 colors.  Which, in my opinion, is a great resolution for 8-bit based computers.

### PICO_SDK_PATH
Make sure you set the `PICO_SDK_PATH` either in the `CMakeLists.txt` file or your local OS environment variables.  This is required to build the project.

### Features
* 320x240 resolution
* 64 colors
* 5x7 pixel font tied to 8x8 boundries.  This allows for 40x30 characters on the screen.
* Simple buffer for drawing.  Just draw to the buffer and everything is blitted in real time.
* 6-bit RGB color palette of R2G2B2.  This allows for 64 colors.
* One byte per pixel (upper two bits are ignored).

### Issues
* There are plenty!  This is a work in progress.
* I'm not sure why changing the `drawPixel` bound checks for X will cause all of the colors to be messed up and odd behaviors.

### Future
* Add support for scrolling text.
* Add support for sprites.
* Add support for scrolling.
* Etc...
