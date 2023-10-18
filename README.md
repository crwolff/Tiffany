# Tiffany [![Development build](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml/badge.svg)](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml)


A Specialized tool for editing scanned documents.

Left Mouse Button (function based on toolbar):
* Pointer - Draw selection rectangle
* Pencil/Eraser - Draw or Erase in various pen widths
    * Hold shift to draw straight lines

Right Mouse Button:
* Zoom to rectangle
* Shift - pan image

Keyboard:
* 'F' - Apply most recent toolbar zoom (default: zoom to fit)
* Cut ('^X') - Fill selection with background color
* Shift-Cut - Fill selection with foreground color
* '^S' - Fill outside selection with background color
* Shift-'^S' - Fill outside selection with foreground color
* Undo ('^Z') - Undo previous edit
* Shift-Undo - Redo previous undo
* Copy - Copy selection
* Paste - Paste copied item
    * Click LMB to paste 
    * Holding control snaps image to 'best' nearby location
    * Repeating Paste command cycles through list of copied items.
    * Pressing control without moviing the mouse snaps the image (better for large pastes)
* Shift-Paste - Transparent paste
    * Hold shift while clicking to paste white regions (>F0F0F0) as transparent

## To compile:
```
% qmake
% make
```
