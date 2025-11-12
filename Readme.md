# Repeater64
Collection of 4 small N64 demos that are hard to emulate.
ROM and reference footage can be found under releases.

## Demo 1 - Syncs
Draws an image to the screen in such a way that on console you see a different image than on an emulator.<br> 
This works by drawing the image pixel by pixel with color-rectangles in fill-mode.<br>
Since the RDP requires syncs before changing the fill-color, we can purposely misuse that.<br>
Emulators will show the initial color, whereas on console you will see the color set after the rectangle commmand.

## Demo 2 - Repeat Mode
Draws graphics with the CPU using the MI-Repeat mode (https://n64brew.dev/wiki/MIPS_Interface#0x0430_0000_-_MI_MODE).
This will draw a single value to the screen, whereas the repeat mode fills in a larger line repeating the value.
Emulators failing this will show a messed up screen with short horizontal lines.

## Demo 3 - VI Wobble
Draws an image and shifts the image on a line by line basis via VI registers.<br>
This causes the video output hardware to shift/scale the input image while the image is being drawn.<br>
With inaccurate emulators, this will simply draw a static image.

## Demo 4 - VI Pong
Same effect as Demo 3, but makes a little game out of it.<br>
This is a vertical version of pong, where the paddles and ball are drawn fixed on the left side of the screen.<br>
Movement is done by shifting via VI registers.<br>
An inaccurate emulator will simply show a static image here as well.
