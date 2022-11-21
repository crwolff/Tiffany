// ViewData.h

#ifndef ViewDATA_H
#define ViewDATA_H

class ViewData
{
public:
    ViewData() {}
    ~ViewData() {}

    float scaleFactor = 0.0;
    float scaleBase = 0.0;
    int horizontalScroll = 0;
    int verticalScroll = 0;
};

Q_DECLARE_METATYPE(ViewData)
#endif // ViewDATA_H
