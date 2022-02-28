# Tiffany

Using QGraphicsView as main viewing window. Scaling/Zooming/Panning are nice, 
but it eats memory faster than the garbage collection when using the pencil or eraser.
  
## To compile:
```
% pyrcc5 rsrc.qrc -o rsrc_rc.py
% pyuic5 mainWin.ui -o mainWin.py
% python3 tiffany.py
```
