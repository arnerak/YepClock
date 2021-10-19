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

#pragma once

#include <stdint.h>

#define LOCHAR ' '
#define HICHAR '~'
#define NUMCHARS (HICHAR - LOCHAR + 1)

#define FBM_BOLD 1
#define FBM_ITALIC 2

struct Glyph
{
    float u, v;
    int8_t charWidth, charHeight, advance, xoff, yoff;
};

class FontBitmap
{
public:
    FontBitmap();
    ~FontBitmap();

    void create(const char* fontName, int fontSize, int flags);

    const Glyph& getGlyph(char c) const;
    const int getFontHeight() const { return fontHeight; }
    const int getFontHeightAboveBaseline() const { return fontHeight - fontDescent; }
    const int getFontAscent() const { return fontAscent; }
    const int getFontDescent() const { return fontDescent; }
    const int getTextureWidth() const { return textureWidth; }
    const int getGlyphWidth() const { return glyphWidth; }
    const int getGlyphHeight() const { return glyphHeight; }
    const float getUW() const { return glyphWidth / (float)textureWidth; }
    const float getVH() const { return glyphHeight / (float)textureWidth; }

    const unsigned int getGLTexture() const { return texture; }

private:
    FontBitmap(const FontBitmap&);

    Glyph glyphs[NUMCHARS];

    unsigned int texture;

    int fontHeight, fontAscent, fontDescent, textureWidth, glyphWidth, glyphHeight;
};
