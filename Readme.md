# Repeater64
Collection of a few small N64 demos that are hard to emulate.<br>
ROM and reference footage can be found under releases.

**Note:** Please run this ROM in NTSC mode, otherwise VI effect may fail.

[![Watch the video](https://img.youtube.com/vi/bRw4pr9IZL4/hqdefault.jpg)](https://www.youtube.com/embed/bRw4pr9IZL4)

# Demos
Explanations of each demo and what they are testing:

## RDRAM 9th Bit
The RDRAM on the N64 actually contains 9 bits per byte.<br>
Usually you would use this for e.g. error correction, but the N64 repurposes it for triangle coverage.<br>
Each memory write on CPU/RSP/RDP will set this bit based on the value written,<br> 
and the RDP also sets it explicitly when actual coverage is written out.<br>
The only way to access it via the CPU is to enter a special mode.<br>

This demo uses those 9th bits to store specific values later used in an image blitting process.<br>
The purpose of it is simply to screw with color and offsets, which have been intentionally altered beforehand.<br>
If incorrect or missing bits are present, the image will simply display garbage.<br>

The source of those bits come form both the RDP (fill-mode rectangles), and specific regular CPU writes.<br>

## RDP Fill-Mode Triangles
When the RDP is operated under fill-mode (fast drawing of a single color), only rectangles are allowed.<br>
Using Triangle can result in them looking corrupt or even hanging up the RDP entirely.<br>
Which depends on their shape and handedness, where they either look perfectly fine, have repeating patterns and gaps, or crash.<br>
This demo draws a sequence of non-crashing triangles and compares them against a known good reference.

## RDP 1-Cycle No-Sync
When drawing shapes that use env-color, a sync is needed before doing so.<br>
If the sync is missing, the last few pixels of the last shape can already use one of the newer color.<br>
This demo draws a sequence of rectangles and compares them against a known good reference.

## RDP Fill-Mode Syncs
Draws an image to the screen in such a way that on console you see a different image than on an emulator.<br> 
This works by drawing the image pixel by pixel with color-rectangles in fill-mode.<br>
Since the RDP requires syncs before changing the fill-color, we can purposely misuse that.<br>
Emulators will show the initial color, whereas on console you will see the color set after the rectangle commmand.

## RDP Test-Mode
The RDP contains a special test mode you can put it in.<br>
With it enabled, reading out the span-buffers is possible.<br>
Span buffers are internal buffers the RDP uses to store pixel data before outputting it to the screen.<br>
By reading out these buffers, we can mostly reconstruct a line of pixels as they are being drawn.<br>
In this demo the RDP draws a given triangle line by line (scissor rect), and each time the CPU fetches and parses this data.<br>
Lastly, color and coverage data is then visualized as text / ASCII art.<br>

Emulators that do not handle this will simply display nothing.<br>
This is also a hard thing to emulate since usually the RDP is done on a GPU and in parallel to the CPU.<br>

In addition, it is possible to write to the span-buffers in test-mode.<br>
Which also performs some specific masking of data.<br>
If that fails, a message is displayed on screen.

## RDP Undefined Shade
When using the shade input in the color-combiner, the command that draws a shape must set this value.<br>
The only command that can do is a triangle command with the shading info attached.<br>
If it is not set, garbage values will be used irrespective of any previous state.<br>
This seems to be dependent on some of the triangle slopes itself.<br>
This demo draws a sequence of unshaded triangles and compares them against a known good reference.

## MI-Repeat Mode
Draws graphics with the CPU using the MI-Repeat mode (https://n64brew.dev/wiki/MIPS_Interface#0x0430_0000_-_MI_MODE).<br>
Repeat mode causes a single CPU write to be duplicated multiple times, effectively forming lines if used for drawing.<br>
This demo uses it to draw graphics and text.<br>
Emulators failing this will show a messed up screen with short horizontal lines.

## VI Pre-Line Effects
Draws an image and shifts the image on a line by line basis via VI registers.<br>
This causes the video output hardware to shift/scale the input image while the image is being drawn.<br>
With inaccurate emulators, this will simply draw a static image.

## VI Pong
Same effect as the other VI demo, but makes a little game out of it.<br>
This is a vertical version of pong, where the paddles and ball are drawn fixed on the left side of the screen.<br>
Movement is done by shifting via VI registers.<br>
An inaccurate emulator will simply show a static image here as well.
