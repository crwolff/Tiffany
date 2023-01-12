# Tiffany [![Development build](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml/badge.svg)](https://github.com/crwolff/Tiffany/actions/workflows/build_all.yml)


A Specialized tool for editting scanned documents.

Left Mouse Button:
* pointer to draw selection rectangle
* Draw in foreground color
* Dropper to select all pixels within threshold of current one (Cut to erase them)
* Deskew tool (rotate using spinbox on toolbar, Paste to fix image)

Right Mouse Button:
* Zoom to rectangle
* Shift - pan image

Esc - cancel paste operation
^C - Copy from selection rectangle
^V - Drag copied data around with mouse, LMB to place
^Z - Undo previous edit
^X - Fill area with background color
^S - Fill outside area with background color
Shift-^Z - Redo previous undo

## To compile:
```
% qmake
% make
```
