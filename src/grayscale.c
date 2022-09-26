#include "grayscale.h"

#include "color.h"

// 0x596D90
unsigned char _GreyTable[256];

// 0x44FA78
void grayscalePaletteUpdate(int a1, int a2)
{
    if (a1 >= 0 && a2 <= 255) {
        for (int index = a1; index <= a2; index++) {
            // NOTE: The only way to explain so much calls to [Color2RGB] with
            // the same repeated pattern is by the use of min/max macros.

            int v1 = max((Color2RGB(index) & 0x7C00) >> 10, max((Color2RGB(index) & 0x3E0) >> 5, Color2RGB(index) & 0x1F));
            int v2 = min((Color2RGB(index) & 0x7C00) >> 10, min((Color2RGB(index) & 0x3E0) >> 5, Color2RGB(index) & 0x1F));
            int v3 = v1 + v2;
            int v4 = (int)((double)v3 * 240.0 / 510.0);

            int paletteIndex = ((v4 & 0xFF) << 10) | ((v4 & 0xFF) << 5) | (v4 & 0xFF);
            _GreyTable[index] = colorTable[paletteIndex];
        }
    }
}

// 0x44FC40
void grayscalePaletteApply(unsigned char* buffer, int width, int height, int pitch)
{
    unsigned char* ptr = buffer;
    int skip = pitch - width;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char c = *ptr;
            *ptr++ = _GreyTable[c];
        }
        ptr += skip;
    }
}
