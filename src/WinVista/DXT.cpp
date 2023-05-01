/*
* This file is part of FshThumbnailHandler, a Windows thumbnail handler for FSH images.
*
* Copyright (c) 2009, 2010, 2012, 2013, 2023 Nicholas Hayes
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
// This code has been adapted from libsquish
/*
* Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to	deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "DXT.h"

static int Unpack565(const unsigned char* packed, unsigned char* colors)
{
    int value = packed[0] | (packed[1] << 8);

    int red = (value >> 11) & 0x1f;
    int green = (value >> 5) & 0x3f;
    int blue = (value & 0x1f);

    colors[0] = ((red << 3) | (red >> 2));
    colors[1] = ((green << 2) | (green >> 4));
    colors[2] = ((blue << 3) | (blue >> 2));
    colors[3] = 255;

    return value;
}

static void DecompressColor(unsigned char* rgba, const unsigned char* block, bool isDxt1)
{
    unsigned char codes[16];

    int a = Unpack565(block, codes);
    int b = Unpack565(block + 2, codes + 4);

    // unpack the midpoints
    for (int i = 0; i < 3; i++)
    {
        int c = codes[i];
        int d = codes[4 + i];

        if (isDxt1 && a <= b) // dxt1 alpha is a special case
        {
            codes[8 + i] = ((c + d) / 2);
            codes[12 + i] = 0;
        }
        else
        {
            // handle the other mask cases from FSHTool.

            if (a > b)
            {
                codes[8 + i] = ((2 * c + d) / 3);
                codes[12 + i] = ((c + 2 * d) / 3);
            }
            else
            {
                codes[8 + i] = ((c + d) / 2);
                codes[12 + i] = ((c + d) / 2);
            }
        }
    }

    // fill in alpha for the intermediate values
    codes[8 + 3] = 255;
    codes[12 + 3] = (isDxt1 && a <= b) ? 0 : 255;

    unsigned char indices[16];

    for (int i = 0; i < 4; i++)
    {
        unsigned char* index = indices + 4 * i;
        unsigned char packed = block[4 + i];

        index[0] = (packed & 3);
        index[1] = ((packed >> 2) & 3);
        index[2] = ((packed >> 4) & 3);
        index[3] = ((packed >> 6) & 3);
    }
    // store out the colors
    for (int i = 0; i < 16; i++)
    {
        int offset = 4 * indices[i];
        int index = 4 * i;
        for (int j = 0; j < 4; j++)
        {
            rgba[index + j] = codes[offset + j];
        }
    }
}

static void DecompressDXT3Alpha(unsigned char* rgba, const unsigned char* block)
{
    for (int i = 0; i < 8; i++)
    {
        unsigned char quant = block[i];

        // extract the values
        int lo = quant & 0x0f;
        int hi = quant & 0xf0;
        int index = 8 * i;
        // convert back up to unsigned chars
        rgba[index + 3] = (lo | (lo << 4));
        rgba[index + 7] = (hi | (hi >> 4));
    }
}

static void Decompress(unsigned char* rgba, const unsigned char* block, bool dxt1)
{
    const unsigned char* colorBlock = block;
    const unsigned char* alphaBlock = block;

    if (dxt1)
    {
        DecompressColor(rgba, colorBlock, true);
    }
    else
    {
        colorBlock = block + 8;
        DecompressColor(rgba, colorBlock, false);
        DecompressDXT3Alpha(rgba, alphaBlock);
    }
}

void DecompressImage(unsigned char* rgba, int width, int height, const unsigned char* blocks, bool dxt1)
{
    unsigned char targetRGBA[4 * 16];

    const int bytesPerBlock = dxt1 ? 8 : 16;
    const unsigned char* block = blocks;

    unsigned char* sourcePixel;
    unsigned char* targetPixel;

    for (int y = 0; y < height; y += 4)
    {
        for (int x = 0; x < width; x += 4)
        {
            // decompress the block.
            Decompress(targetRGBA, block, dxt1);

            // write the decompressed pixels to the correct image locations
            sourcePixel = targetRGBA;
            for (int py = 0; py < 4; py++)
            {
                int sy = y + py;

                for (int px = 0; px < 4; px++)
                {
                    // get the target location
                    int sx = x + px;

                    if (sx < width && sy < height)
                    {
                        targetPixel = rgba + 4 * ((width * sy) + sx);

                        for (int p = 0; p < 4; p++)
                        {
                            *targetPixel++ = *sourcePixel++; // copy the target value
                        }
                    }
                    else
                    {
                        // skip the pixel as its outside the range
                        sourcePixel += 4;
                    }
                }
            }

            block += bytesPerBlock;
        }
    }
}
