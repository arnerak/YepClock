/*
Copyright (c) 2021 Arne Rak

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would
   be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
*/

#include "fontbitmap.h"
#include <glad/glad.h>
#include <Windows.h>
#include <math.h>

FontBitmap::FontBitmap()
    : glyphHeight(0)
    , glyphWidth(0)
{
}

int roundUpToPowerOfTwo(int num)
{
    int res = 1;

    if (num > 0)
    {
        while (res < num)
            res *= 2;
    }
    else
    {
        int res = -1;
        while (res > num)
            res *= 2;
    }

    return res;
}


void FontBitmap::create(const char* fontName, int fontSize, int flags)
{
    // create hdc
    auto hdc = CreateCompatibleDC(NULL);

    // create font
    const int cHeight = -fontSize * GetDeviceCaps(hdc, LOGPIXELSY) / 72;
    auto hFont = CreateFontA(cHeight, 0, 0, 0, 500, (flags & FBM_ITALIC), 0, 0,
        DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);
    auto hFontOld = SelectObject(hdc, hFont);

    // get maximum character height
    TEXTMETRICA tm;
    GetTextMetricsA(hdc, &tm);
    fontHeight = tm.tmHeight;
    fontAscent = tm.tmAscent;
    fontDescent = tm.tmDescent;

    // find max glyph width and height
    GLYPHMETRICS glyphMetrics;
    MAT2 mat;
    memset(&mat, 0, sizeof(mat));
    mat.eM11.value = 1;
    mat.eM22.value = 1;
    for (int i = 0; i < NUMCHARS; i++)
    {
        GetGlyphOutlineA(hdc, LOCHAR + i, GGO_GRAY8_BITMAP, &glyphMetrics, 0, NULL, &mat);
        glyphWidth = max(glyphWidth, glyphMetrics.gmBlackBoxX);
        glyphHeight = max(glyphHeight, glyphMetrics.gmBlackBoxY);
    }
    // pad by one pixel to avoid glyph overlap in font bitmap
    glyphWidth++;
    glyphHeight++;

    // calculate needed font bitmap texture size
    const float averageTextureWidth = sqrt(NUMCHARS);
    const float ratio = sqrt(glyphHeight / (float)glyphWidth);
    const int numCols = ceil(averageTextureWidth * ratio);
    const int numRows = ceil(averageTextureWidth / ratio);
    textureWidth = roundUpToPowerOfTwo(max(numCols * glyphWidth, numRows * glyphHeight));

    // allocate texture
    const size_t textureBufSize = textureWidth * textureWidth;
    BYTE* fontTexture = (BYTE*)malloc(textureBufSize);
    memset(fontTexture, 0, textureBufSize);

    // allocate temp glyph texture
    const size_t glyphBufSize = glyphHeight * glyphWidth;
    BYTE* glyphBuf = (BYTE*)malloc(glyphBufSize);
    memset(glyphBuf, 0, glyphBufSize);

    // generate glyph textures
    int8_t minYOffset = 64;
    for (int i = 0; i < NUMCHARS; i++)
    {
        GetGlyphOutlineA(hdc, LOCHAR + i, GGO_GRAY8_BITMAP, &glyphMetrics, glyphBufSize, glyphBuf, &mat);
        const int glyphX = (i % numCols) * glyphWidth;
        const int glyphY = (i / numCols) * glyphHeight;
        int index = 0;
        // copy glyph texture into font bitmap texture
        for (int y = 0; y < glyphMetrics.gmBlackBoxY; y++)
        {
            for (int x = 0; x < glyphMetrics.gmBlackBoxX; x++)
            {
                BYTE color = 255 * glyphBuf[index++] / 64;
                fontTexture[glyphX + x + (glyphY + y) * textureWidth] = color;
            }
            index += (4 - (glyphMetrics.gmBlackBoxX % 4)) % 4;
        }
        glyphs[i].advance = glyphMetrics.gmCellIncX;
        glyphs[i].charWidth = glyphMetrics.gmBlackBoxX;
        glyphs[i].charHeight = glyphMetrics.gmBlackBoxY;
        glyphs[i].u = glyphX / (float)textureWidth;
        glyphs[i].v = glyphY / (float)textureWidth;
        glyphs[i].xoff = glyphMetrics.gmptGlyphOrigin.x;
        glyphs[i].yoff = tm.tmAscent - glyphMetrics.gmptGlyphOrigin.y;
        minYOffset = min(minYOffset, glyphs[i].yoff);
        //_printf("%c [%f %f] [%f %f]\n", LOCHAR + i, glyphs[i].u, glyphs[i].v, glyphs[i].u + getUW(), glyphs[i].v + getVH());
    }
    free(glyphBuf);

    // normalize y offset
    fontHeight -= minYOffset;
    fontAscent -= minYOffset;
    for (int i = 0; i < NUMCHARS; i++)
    {
        glyphs[i].yoff -= minYOffset;
    }

    // these were padded by one pixel to avoid glyph overlap, undo.
    glyphWidth -= 1;
    glyphHeight -= 1;

    // create directx texture and shared resource view
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, textureWidth, textureWidth, 0, GL_RED, GL_UNSIGNED_BYTE, fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);


    // free stuff
    free(fontTexture);
    SelectObject(hdc, hFontOld);
    DeleteObject(hFont);
    DeleteDC(hdc);
}

FontBitmap::~FontBitmap()
{
    if (texture) glDeleteTextures(1, &texture);
}

const Glyph& FontBitmap::getGlyph(char c) const
{
    c = (c < LOCHAR || c > HICHAR) ? '?' : c;
    return glyphs[c - LOCHAR];
}
