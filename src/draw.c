#include "draw.h"

#include <string.h>

#include "color.h"
#include "core.h"
#include "mmx.h"

// 0x4D2FC0
void bufferDrawLine(unsigned char* buf, int pitch, int x1, int y1, int x2, int y2, int color)
{
    int temp;
    int dx;
    int dy;
    unsigned char* p1;
    unsigned char* p2;
    unsigned char* p3;
    unsigned char* p4;

    if (x1 == x2) {
        if (y1 > y2) {
            temp = y1;
            y1 = y2;
            y2 = temp;
        }

        p1 = buf + pitch * y1 + x1;
        p2 = buf + pitch * y2 + x2;
        while (p1 < p2) {
            *p1 = color;
            *p2 = color;
            p1 += pitch;
            p2 -= pitch;
        }
    } else {
        if (x1 > x2) {
            temp = x1;
            x1 = x2;
            x2 = temp;

            temp = y1;
            y1 = y2;
            y2 = temp;
        }

        p1 = buf + pitch * y1 + x1;
        p2 = buf + pitch * y2 + x2;
        if (y1 == y2) {
            memset(p1, color, p2 - p1);
        } else {
            dx = x2 - x1;

            int v23;
            int v22;
            int midX = x1 + (x2 - x1) / 2;
            if (y1 <= y2) {
                dy = y2 - y1;
                v23 = pitch;
                v22 = midX + ((y2 - y1) / 2 + y1) * pitch;
            } else {
                dy = y1 - y2;
                v23 = -pitch;
                v22 = midX + (y1 - (y1 - y2) / 2) * pitch;
            }

            p3 = buf + v22;
            p4 = p3;

            if (dx <= dy) {
                int v28 = dx - (dy / 2);
                int v29 = dy / 4;
                while (true) {
                    *p1 = color;
                    *p2 = color;
                    *p3 = color;
                    *p4 = color;

                    if (v29 == 0) {
                        break;
                    }

                    if (v28 >= 0) {
                        p3++;
                        p2--;
                        p4--;
                        p1++;
                        v28 -= dy;
                    }

                    p3 += v23;
                    p2 -= v23;
                    p4 -= v23;
                    p1 += v23;
                    v28 += dx;

                    v29--;
                }
            } else {
                int v26 = dy - (dx / 2);
                int v27 = dx / 4;
                while (true) {
                    *p1 = color;
                    *p2 = color;
                    *p3 = color;
                    *p4 = color;

                    if (v27 == 0) {
                        break;
                    }

                    if (v26 >= 0) {
                        p3 += v23;
                        p2 -= v23;
                        p4 -= v23;
                        p1 += v23;
                        v26 -= dx;
                    }

                    p3++;
                    p2--;
                    p4--;
                    p1++;
                    v26 += dy;

                    v27--;
                }
            }
        }
    }
}

// 0x4D31A4
void bufferDrawRect(unsigned char* buf, int pitch, int left, int top, int right, int bottom, int color)
{
    bufferDrawLine(buf, pitch, left, top, right, top, color);
    bufferDrawLine(buf, pitch, left, bottom, right, bottom, color);
    bufferDrawLine(buf, pitch, left, top, left, bottom, color);
    bufferDrawLine(buf, pitch, right, top, right, bottom, color);
}

// 0x4D322C
void bufferDrawRectShadowed(unsigned char* buf, int pitch, int left, int top, int right, int bottom, int ltColor, int rbColor)
{
    bufferDrawLine(buf, pitch, left, top, right, top, ltColor);
    bufferDrawLine(buf, pitch, left, bottom, right, bottom, rbColor);
    bufferDrawLine(buf, pitch, left, top, left, bottom, ltColor);
    bufferDrawLine(buf, pitch, right, top, right, bottom, rbColor);
}

// 0x4D33F0
void blitBufferToBufferStretch(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int destWidth, int destHeight, int destPitch)
{
    int heightRatio = (destHeight << 16) / srcHeight;
    int widthRatio = (destWidth << 16) / srcWidth;

    int v1 = 0;
    int v2 = heightRatio;
    for (int srcY = 0; srcY < srcHeight; srcY += 1) {
        int v3 = widthRatio;
        int v4 = (heightRatio * srcY) >> 16;
        int v5 = v2 >> 16;
        int v6 = 0;

        unsigned char* c = src + v1;
        for (int srcX = 0; srcX < srcWidth; srcX += 1) {
            int v7 = v3 >> 16;
            int v8 = v6 >> 16;

            unsigned char* v9 = dest + destPitch * v4 + v8;
            for (int destY = v4; destY < v5; destY += 1) {
                for (int destX = v8; destX < v7; destX += 1) {
                    *v9++ = *c;
                }
                v9 += destPitch;
            }

            v3 += widthRatio;
            c++;
            v6 += widthRatio;
        }
        v1 += srcPitch;
        v2 += heightRatio;
    }
}

// 0x4D3560
void blitBufferToBufferStretchTrans(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int destWidth, int destHeight, int destPitch)
{
    int heightRatio = (destHeight << 16) / srcHeight;
    int widthRatio = (destWidth << 16) / srcWidth;

    int v1 = 0;
    int v2 = heightRatio;
    for (int srcY = 0; srcY < srcHeight; srcY += 1) {
        int v3 = widthRatio;
        int v4 = (heightRatio * srcY) >> 16;
        int v5 = v2 >> 16;
        int v6 = 0;

        unsigned char* c = src + v1;
        for (int srcX = 0; srcX < srcWidth; srcX += 1) {
            int v7 = v3 >> 16;
            int v8 = v6 >> 16;

            if (*c != 0) {
                unsigned char* v9 = dest + destPitch * v4 + v8;
                for (int destY = v4; destY < v5; destY += 1) {
                    for (int destX = v8; destX < v7; destX += 1) {
                        *v9++ = *c;
                    }
                    v9 += destPitch;
                }
            }

            v3 += widthRatio;
            c++;
            v6 += widthRatio;
        }
        v1 += srcPitch;
        v2 += heightRatio;
    }
}

// 0x4D36D4
void blitBufferToBuffer(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch)
{
    mmxBlit(dest, destPitch, src, srcPitch, width, height);
}

// 0x4D3704
void blitBufferToBufferTrans(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch)
{
    mmxBlitTrans(dest, destPitch, src, srcPitch, width, height);
}

// 0x4D387C
void bufferFill(unsigned char* buf, int width, int height, int pitch, int a5)
{
    int y;

    for (y = 0; y < height; y++) {
        memset(buf, a5, width);
        buf += pitch;
    }
}

// 0x4D38E0
void _buf_texture(unsigned char* buf, int width, int height, int pitch, void* a5, int a6, int a7)
{
    // TODO: Incomplete.
}

// 0x4D3A48
void _lighten_buf(unsigned char* buf, int width, int height, int pitch)
{
    int skip = pitch - width;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char p = *buf;
            *buf++ = intensityColorTable[(p << 8) + 147];
        }
        buf += skip;
    }
}

// Swaps two colors in the buffer.
//
// 0x4D3A8C
void _swap_color_buf(unsigned char* buf, int width, int height, int pitch, int color1, int color2)
{
    int step = pitch - width;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int v1 = *buf & 0xFF;
            if (v1 == color1) {
                *buf = color2 & 0xFF;
            } else if (v1 == color2) {
                *buf = color1 & 0xFF;
            }
            buf++;
        }
        buf += step;
    }
}

// 0x4D3AE0
void bufferOutline(unsigned char* buf, int width, int height, int pitch, int color)
{
    unsigned char* ptr = buf + pitch;

    bool cycle;
    for (int y = 0; y < height - 2; y++) {
        cycle = true;

        for (int x = 0; x < width; x++) {
            if (*ptr != 0 && cycle) {
                *(ptr - 1) = color & 0xFF;
                cycle = false;
            } else if (*ptr == 0 && !cycle) {
                *ptr = color & 0xFF;
                cycle = true;
            }

            ptr++;
        }

        ptr += pitch - width;
    }

    for (int x = 0; x < width; x++) {
        ptr = buf + x;
        cycle = true;

        for (int y = 0; y < height; y++) {
            if (*ptr != 0 && cycle) {
                // TODO: Check in debugger, might be a bug.
                *(ptr - pitch) = color & 0xFF;
                cycle = false;
            } else if (*ptr == 0 && !cycle) {
                *ptr = color & 0xFF;
                cycle = true;
            }

            ptr += pitch;
        }
    }
}
