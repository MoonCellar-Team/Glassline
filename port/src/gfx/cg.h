#pragma once

#include <stdint.h>

#include <cairo.h>

#include <memory>
#include <string>
#include <vector>

#include "port/cf.h"
#include "sdk/CoreGraphics.h"

namespace port {
namespace gfx {

struct FontData;

std::shared_ptr<FontData> fontFromBytes(std::vector<uint8_t> bytes);
std::shared_ptr<FontData> fontFromFile(const std::string& path, int index);
int unitsPerEm(FontData& f);
int ascent(FontData& f);
int descent(FontData& f);
int leading(FontData& f);
int advance(FontData& f, unsigned glyph);
unsigned glyphForChar(FontData& f, unsigned codepoint);
CGRect bbox(FontData& f);
cairo_font_face_t* cairoFace(FontData& f);

std::vector<uint8_t> providerBytes(void* provider);

void showGlyphs(CGContext* c, FontData& font, float size, const float matrix[4], float x, float y,
                const unsigned short* glyphs, int count);

void colorComponents(const void* color, float rgba[4]);

CGImage* makeImage(int width, int height);
uint8_t* imagePixels(CGImage* image);
int imageWidth(CGImage* image);
int imageHeight(CGImage* image);
cairo_surface_t* imageSurface(CGImage* image);

}
}
