# Tiffany [![Development build](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml/badge.svg)](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml)


A Specialized tool for editing scanned documents. Some buttons will perform the operation
on all pages selected in the bookmarks tab. These are indicated with (button<sup>m</sup>).

* Open - Open files
    * Open - Open new files at end of list
    * Insert - Insert new files before selection
    * Replace - Replace all selected items with new files
* Save<sup>m</sup> - Save files
    * Save - Replace changed files in selection
    * Save To -  Save all files in selection to new directory
* Delete<sup>m</sup> - Remove selection from list (doesn't remove from directory)
* Blank Page<sup>m</sup> - Fill page with background and insert foreground colored text into center
* Rotate<sup>m</sup>
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
* Color button
    * Click upper left of icon to set background color
    * Click upper right of icon to swap foreground/background colors
    * Click lower left of icon to reset colors to Black/White
    * Click lower right of icon to set foreground color
* Pointer - Draw selection rectangle
* Pencil/Eraser - Draw (foreground) or Erase (background) in various pen widths
    * Hold shift to draw straight lines
    * Left click without moving to draw dot
    * 'T' toggles the pencil orientation
* Dropper button - Adjacent spinbox controls thresholds for different functions
    * Dropper - Select all pixels within threshold of selected color
    * Flood Fill - Select all adjacent pixels within threshold of selected color
    * BG Remove<sup>m</sup> - Select all pixels within threshold of White
* Despeckle button - Adjacent spinbox controls blob size
    * Despeckle<sup>m</sup> - Select all groups of non-white pixels below a certain size
    * Devoid<sup>m</sup> - Select all groups of non-black pixels below a certain size
* Deskew<sup>m</sup> - Rotate page so text is horizontal
* Format button - Converts image format
    * Binary<sup>m</sup> - Converts image to binary using Otsu's global threshold (good for large dark areas)
    * Adaptive<sup>m</sup> - Converts image to binary using an adaptive threshold (usually good)
    * Grayscale<sup>m</sup> - Converts image to grayscale
    * Dithered<sup>m</sup> - Converts image to dithered binary (good for grayscale images)

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
