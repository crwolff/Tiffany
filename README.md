# Tiffany [![Development build](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml/badge.svg)](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml)


A Specialized tool for editing scanned documents.

* Open - Open files
    * Open - Open new files at end of list
    * Insert - Insert new files before selection
    * Replace - Replace all selected items with new files
* Save - Save files
    * Save - Replace changed files in selection
    * Save To -  Save all files in selection to new directory
* Delete - Remove selection from list (doesn't remove from directory)
* Rotate
    * Rotate CW - Rotates selected images 90 degrees
    * Rotate CCW - Rotates selected images -90 degrees
    * Rotate 180 - Turns selected images upside down
    * Horizontal Mirror - Mirrors selected images about vertical line
    * Vertical Mirror - Mirrors selected images about horizontal line
* Undo/Redo
    * Undo - Undo previous edit on active page
    * Redo - Redo previous undo on active page
* Zoom
    * Zoom In - Increase magnification 25%
    * Zoom Out - Decrease magnifaction 20%
    * Zoom Fit - Adjust zoom so entire page fits in window
    * Zoom Fill - Adjust zoom so entire window is filled
    * Fit Width - Adjust zoom so full width of window is filled
    * Fit Height - Adjust zoom so full height of window is filled
* Mode
    * (S)ingle Page Mode - Buttons only operate on active page
    * (M)ulti-page Mode - Certain buttons operate on all selected pages

Left Mouse Button (function based on toolbar):
* Pointer - Draw selection rectangle
* Pencil/Eraser - Draw or Erase in various pen widths
    * Hold shift to draw straight lines
* Dropper - Recolor all pixels within threshold of selected color
* Flood Fill - Recolor all adjacent pixels within threshold of selected color

Multi-page tools:
* Dropper button
    * BG Remove - Recolor all pixels within threshold of White
    * Blank Page - Fill page with background and insert text into center

Color button:
* Click upper left to set background color
* Click upper right to swap foreground/background colors
* Click lower left to reset colors to Black/White
* Click lower right to set foreground color

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
