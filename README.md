# VgaPico
This is a VGA library for the Raspberry Pi Pico. It is based on the Hunter Adams code.

The intention is to be easy to learn and provide 320x240 resolution with 64 colors.  Which, in my opinion, is a great resolution for 8-bit based computers.

### PICO_SDK_PATH
Make sure you set the `PICO_SDK_PATH` either in the `CMakeLists.txt` file or your local OS environment variables.  This is required to build the project.

### Useful videos 
https://www.youtube.com/watch?v=KCGy7Gz1QrE


### Features
* 320x240 resolution
* 64 colors
* 5x7 pixel font tied to 8x8 boundries.  This allows for 40x30 characters on the screen.  (original code)
* Added PETSCII font with true 8x8 character drawing and each character can have their own 64 color background and foreground!
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


### Wiring
* HSYNC = GPIO14
* VSYNC = GPIO15
* RED = GPIO2 & GPIO3
* GREEN = GPIO4 & GPIO5
* BLUE = GPIO6 & GPIO7


## Cool demos!
![demo01.jpg](images%2Fdemo01.jpg)

![demo02.jpg](images%2Fdemo02.jpg)

![demo03.jpg](images%2Fdemo03.jpg)

## Not completely working sprites (lol)
![sprites.jpg](images%2Fsprites.jpg)

### Commodore PETSCII Font!!
![demo04.jpg](images%2Fdemo04.jpg)


## Fonts
Here is a good site to get different PETSCII fonts:
http://www.6502.org/users/sjgray/computer/cbmchr/cbmchr.html

Also, checkout the [petscii.py](utils/petscii.py) file for a crud program to convert binary font data into a "petscii.h" include file.

The main PETSCII file is called (chargen) and it can be found on the Web (also included in the petscii folder).

