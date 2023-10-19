# Tiffany [![Development build](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml/badge.svg)](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml)


A Specialized tool for editing scanned documents.

Left Mouse Button (function based on toolbar):
* Pointer - Draw selection rectangle
* Pencil/Eraser - Draw or Erase in various pen widths
    * Hold shift to draw straight lines
* Dropper - Recolor all pixels within threshold of selected color
* BG Remove - Recolor all pixels within threshold of White (multi-page)

Right Mouse Button:
* Zoom to rectangle
* Shift - pan image

Keyboard:
* Escape - Cancels current command (Paste, etc)
* 'F' - Apply most recent toolbar zoom (default: zoom to fit)
* Cut ('^X') - Depends on tool
    * Pointer - Fill selection with background color
    * Dropper - Set blinking pixels to background color
    * BG Remove - Set blinking pixels to background color
* Shift-Cut - Depends on tool
    * Pointer - Fill outside selection with background color
* '^S' - Depends on tool
    * Pointer - Fill selection with foreground color
    * Dropper - Set blinking pixels to foreground color
    * BG Remove - Set blinking pixels to foreground color
* Shift-'^S' - Depends on tool
    * Pointer - Fill outside selection with foreground color
* Undo ('^Z') - Undo previous edit
* Shift-Undo - Redo previous undo
* Copy - Copy selection
* Paste - Paste copied item
    * Click LMB to paste 
    * Holding control snaps image to 'best' nearby location
    * Repeating Paste command cycles through list of copied items
    * Pressing Control without moving the mouse snaps the image (better for large pastes)
    * Pressing Delete while pasting removes current image from copied item list
* Shift-Paste - Transparent paste
    * Hold shift while clicking to paste white regions (\>#F0F0F0) as transparent

## To compile:
```
% qmake
% make
```
