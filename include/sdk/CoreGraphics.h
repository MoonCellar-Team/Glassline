#pragma once

extern "C" {

struct CGContext;
struct CGImage;
typedef CGImage* CGImageRef;

struct CGPoint {
    float x;
    float y;
};

struct CGSize {
    float width;
    float height;
};

struct CGRect {
    CGPoint origin;
    CGSize size;
};

typedef unsigned long (*CGDataProviderGetBytesCallback)(void* info, void* buffer, unsigned long count);
typedef long long (*CGDataProviderSkipForwardCallback)(void* info, long long count);
typedef void (*CGDataProviderRewindCallback)(void* info);
typedef void (*CGDataProviderReleaseInfoCallback)(void* info);

struct CGDataProviderSequentialCallbacks {
    unsigned int version;
    CGDataProviderGetBytesCallback getBytes;
    CGDataProviderSkipForwardCallback skipForward;
    CGDataProviderRewindCallback rewind;
    CGDataProviderReleaseInfoCallback releaseInfo;
};

void CGContextBeginPath(CGContext* c);
void CGContextMoveToPoint(CGContext* c, float x, float y);
void CGContextAddCurveToPoint(CGContext* c, float cp1x, float cp1y, float cp2x, float cp2y,
                              float x, float y);
void* CGDataProviderCreateSequential(void* info, const CGDataProviderSequentialCallbacks* callbacks);
CGImageRef CGBitmapContextCreateImage(CGContext* context);
void CGImageRelease(CGImageRef image);
void CGContextRelease(CGContext* context);
void* CGColorSpaceCreateDeviceRGB();
CGContext* CGBitmapContextCreate(void* data, unsigned long width, unsigned long height,
                                 unsigned long bitsPerComponent, unsigned long bytesPerRow,
                                 void* colorSpace, unsigned int bitmapInfo);
unsigned long CGImageGetWidth(CGImageRef image);
unsigned long CGImageGetHeight(CGImageRef image);
void CGContextSaveGState(CGContext* context);
void CGContextRestoreGState(CGContext* context);
void CGContextTranslateCTM(CGContext* context, float tx, float ty);
void CGContextScaleCTM(CGContext* context, float sx, float sy);
void CGContextDrawImage(CGContext* context, CGRect rect, CGImageRef image);
CGImageRef CGImageCreateWithPNGDataProvider(void* provider, const float* decode, bool interpolate,
                                            int intent);
void CGDataProviderRelease(void* provider);
CGContext* CGContextRetain(CGContext* context);
void CGContextSetBlendMode(CGContext* context, int mode);
void CGContextSetInterpolationQuality(CGContext* context, int quality);
void CGContextSetAllowsAntialiasing(CGContext* context, bool allows);
void CGContextSetRGBFillColor(CGContext* context, float r, float g, float b, float a);
void CGContextSetRGBStrokeColor(CGContext* context, float r, float g, float b, float a);
void CGContextClipToRect(CGContext* context, CGRect rect);
void CGContextAddLineToPoint(CGContext* context, float x, float y);
void CGContextFillPath(CGContext* context);
void CGContextStrokePath(CGContext* context);
void CGContextStrokeRect(CGContext* context, CGRect rect);
CGImageRef CGImageCreateWithImageInRect(CGImageRef image, CGRect rect);
CGImageRef CGImageRetain(CGImageRef image);
void* CGColorCreate(void* colorSpace, const float* components);
void CGColorRelease(void* color);
void CGColorSpaceRelease(void* colorSpace);
void CGContextSetShadowWithColor(CGContext* context, CGSize offset, float blur, void* color);
void CGContextSetTextDrawingMode(CGContext* context, int mode);
struct CGFont;
CGFont* CGFontCreateWithDataProvider(void* provider);
void CGFontRelease(CGFont* font);

}
