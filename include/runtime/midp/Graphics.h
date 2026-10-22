#pragma once

#include "m3g/Transform.h"
#include "runtime/midp/Object.h"
#include "sdk/CoreGraphics.h"

namespace m3g {
class OpenGLES11Renderer;
class Renderer;
class OpenGLES20Renderer;
class ShaderBinding;
}

namespace midp {

class DisplayIPhone;
class Displayable;
class GLES11;
class GLES20;
class Class;
class Image;
class ImageIPhone;
class Font;
class String;

class Graphics : public Object {
public:
    virtual void fillRect(int x, int y, int width, int height) = 0;
    virtual void setClip(int x, int y, int width, int height) = 0;
    virtual void setColor(int argb);
    virtual void clipRect(int x, int y, int width, int height) = 0;
    virtual void drawArc(int x, int y, int width, int height, int startAngle, int arcAngle) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2) = 0;
    virtual void drawRect(int x, int y, int width, int height) = 0;
    virtual void drawRegion(const Image* image, int srcX, int srcY, int width, int height, int transform,
                            int x, int y, int anchor) = 0;
    virtual void fillArc(int x, int y, int width, int height, int startAngle, int arcAngle) = 0;
    virtual void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3) = 0;
    virtual void setColor(int red, int green, int blue);
    virtual int getClipHeight() const = 0;
    virtual int getClipWidth() const = 0;
    virtual int getClipX() const = 0;
    virtual int getClipY() const = 0;
    virtual void setFontDropShadowParameters(int dx, int dy, int blur, int argb);
    virtual void drawString(String* str, int x, int y, int anchor, int style) = 0;
    virtual void drawSubstring(String* str, int offset, int length, int x, int y, int anchor,
                               int style) = 0;
    void setFont(Font* font);
    virtual void drawImage(const Image* image, int x, int y, int anchor) = 0;
    virtual void setColor(int red, int green, int blue, int alpha);
    Font* getFont() const;
    virtual void translate(int x, int y);
    virtual void bind2D() = 0;

    int m_red;
    int m_green;
    int m_blue;
    int m_alpha;
    int m_translateX;
    int m_translateY;
    Font* m_font;

    Graphics();
    virtual ~Graphics();
    virtual Class* getClass() const;
    virtual int getAlphaComponent() const;
    virtual int getBlueComponent() const;
    virtual int getGreenComponent() const;
    virtual int getRedComponent() const;
    virtual int getColor() const;
    virtual int getTranslateX() const;
    virtual int getTranslateY() const;
    virtual void setGrayScale(int gray);
};

class GraphicsIPhoneGLES : public Graphics {
public:
    void setViewport(int x, int y, int rotation);
    void clearViewport();
    GraphicsIPhoneGLES();
    virtual ~GraphicsIPhoneGLES();
    virtual int actualBind2D() = 0;
    virtual m3g::Renderer* createRenderer() = 0;

    int m_unk24;
    int m_unk28;
    int m_unk2c;
    int m_unk30;
    int m_originX;
    int m_originY;
    int m_xAxisX;
    int m_xAxisY;
    int m_yAxisX;
    int m_yAxisY;
};

class GraphicsIPhoneCG : public Graphics {
public:
    explicit GraphicsIPhoneCG(CGContext* context);
    virtual ~GraphicsIPhoneCG();
    virtual void clipRect(int x, int y, int width, int height);
    virtual void copyArea(int x, int y, int width, int height, int dx, int dy, int anchor);
    virtual void drawChar(char c, int x, int y, int anchor);
    virtual void drawChars(char* data, int offset, int length, int x, int y, int anchor);
    virtual void drawRGB(const int* rgb, int offset, int scanlength, int x, int y, int width, int height,
                         bool processAlpha);
    virtual void drawRoundRect(int x, int y, int width, int height, int arcWidth, int arcHeight);
    virtual void setFontDropShadowParameters(int dx, int dy, int blur, int argb);
    virtual void fillRoundRect(int x, int y, int width, int height, int arcWidth, int arcHeight);
    virtual int getClipHeight() const;
    virtual int getClipWidth() const;
    virtual int getClipX() const;
    virtual int getClipY() const;
    virtual int getDisplayColor(int color) const;
    virtual int getGrayScale() const;
    virtual int getStrokeStyle() const;
    virtual void setStrokeStyle(int style);
    virtual void bind2D();
    virtual void setColor(int red, int green, int blue, int alpha);
    virtual void setClip(int x, int y, int width, int height);
    virtual void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
    virtual void fillRect(int x, int y, int width, int height);
    virtual void fillArc(int x, int y, int width, int height, int startAngle, int arcAngle);
    virtual void drawSubstring(String* str, int offset, int length, int x, int y, int anchor, int style);
    virtual void drawString(String* str, int x, int y, int anchor, int style);
    virtual void drawRegion(const Image* image, int srcX, int srcY, int width, int height, int transform,
                            int x, int y, int anchor);
    virtual void drawRect(int x, int y, int width, int height);
    virtual void drawLine(int x1, int y1, int x2, int y2);
    virtual void drawArc(int x, int y, int width, int height, int startAngle, int arcAngle);
    virtual void drawImage(const Image* image, int x, int y, int anchor);

    CGContext* m_context;
    int m_clipX;
    int m_clipY;
    int m_clipWidth;
    int m_clipHeight;
    int m_shadowDx;
    int m_shadowDy;
    int m_shadowBlur;
    int m_shadowArgb;
};

class GraphicsIPhoneGLES11 : public GraphicsIPhoneGLES {
public:
    GraphicsIPhoneGLES11(DisplayIPhone* display, GLES11* gl);
    virtual ~GraphicsIPhoneGLES11();
    virtual void clipRect(int x, int y, int width, int height);
    virtual void copyArea(int x, int y, int width, int height, int dx, int dy, int anchor);
    virtual void drawArc(int x, int y, int width, int height, int startAngle, int arcAngle);
    virtual void drawChar(char c, int x, int y, int anchor);
    virtual void drawChars(char* data, int offset, int length, int x, int y, int anchor);
    virtual void drawImage(const Image* image, int x, int y, int anchor);
    virtual void drawLine(int x1, int y1, int x2, int y2);
    virtual void drawRect(int x, int y, int width, int height);
    virtual void drawRegion(const Image* image, int srcX, int srcY, int width, int height, int transform,
                            int x, int y, int anchor);
    virtual void drawRGB(const int* rgb, int offset, int scanlength, int x, int y, int width, int height,
                         bool processAlpha);
    virtual void drawRoundRect(int x, int y, int width, int height, int arcWidth, int arcHeight);
    virtual void drawString(String* str, int x, int y, int anchor, int style);
    virtual void drawSubstring(String* str, int offset, int length, int x, int y, int anchor, int style);
    virtual void fillArc(int x, int y, int width, int height, int startAngle, int arcAngle);
    virtual void fillRect(int x, int y, int width, int height);
    virtual void fillRoundRect(int x, int y, int width, int height, int arcWidth, int arcHeight);
    virtual void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
    virtual int getClipHeight() const;
    virtual int getClipWidth() const;
    virtual int getClipX() const;
    virtual int getClipY() const;
    virtual int getDisplayColor(int color) const;
    virtual int getGrayScale() const;
    virtual int getStrokeStyle() const;
    virtual void setClip(int x, int y, int width, int height);
    virtual void setStrokeStyle(int style);
    virtual void translate(int x, int y);
    virtual void setColor(int red, int green, int blue, int alpha);
    virtual void bind2D();
    virtual int actualBind2D();
    virtual m3g::Renderer* createRenderer();
    void disableTextures() const;
    void enableTexture(ImageIPhone* image) const;

    DisplayIPhone* m_display;
    GLES11* m_gl;
    mutable ImageIPhone* m_texture;
    mutable bool m_texturesOn;
    bool m_bindPending;
    int m_scissorX;
    int m_scissorY;
    int m_scissorW;
    int m_scissorH;
};

class GraphicsIPhoneGLES20 : public GraphicsIPhoneGLES {
public:
    GraphicsIPhoneGLES20(DisplayIPhone* display, GLES20* gl);
    virtual ~GraphicsIPhoneGLES20();
    void init();
    virtual void clipRect(int x, int y, int width, int height);
    virtual void copyArea(int x, int y, int width, int height, int dx, int dy, int anchor);
    virtual void drawArc(int x, int y, int width, int height, int startAngle, int arcAngle);
    virtual void drawChar(char c, int x, int y, int anchor);
    virtual void drawChars(char* data, int offset, int length, int x, int y, int anchor);
    virtual void drawImage(const Image* image, int x, int y, int anchor);
    virtual void drawLine(int x1, int y1, int x2, int y2);
    virtual void drawRect(int x, int y, int width, int height);
    virtual void drawRegion(const Image* image, int srcX, int srcY, int width, int height, int transform,
                            int x, int y, int anchor);
    virtual void drawRGB(const int* rgb, int offset, int scanlength, int x, int y, int width, int height,
                         bool processAlpha);
    virtual void drawRoundRect(int x, int y, int width, int height, int arcWidth, int arcHeight);
    virtual void drawString(String* str, int x, int y, int anchor, int style);
    virtual void drawSubstring(String* str, int offset, int length, int x, int y, int anchor, int style);
    virtual void fillArc(int x, int y, int width, int height, int startAngle, int arcAngle);
    virtual void fillRect(int x, int y, int width, int height);
    virtual void fillRoundRect(int x, int y, int width, int height, int arcWidth, int arcHeight);
    virtual void fillTriangle(int x1, int y1, int x2, int y2, int x3, int y3);
    virtual int getClipHeight() const;
    virtual int getClipWidth() const;
    virtual int getClipX() const;
    virtual int getClipY() const;
    virtual int getDisplayColor(int color) const;
    virtual int getGrayScale() const;
    virtual int getStrokeStyle() const;
    virtual void setClip(int x, int y, int width, int height);
    virtual void setStrokeStyle(int style);
    virtual void translate(int x, int y);
    virtual void setColor(int red, int green, int blue, int alpha);
    virtual void bind2D();
    virtual int actualBind2D();
    virtual m3g::Renderer* createRenderer();
    void disableTextures() const;
    void enableTexture(ImageIPhone* image) const;
    float* getWVP();

    DisplayIPhone* m_display;
    mutable ImageIPhone* m_texture;
    mutable bool m_texturesOn;
    bool m_bindPending;
    GLES20* m_gl;
    mutable m3g::ShaderBinding* m_shader;
    bool m_needInit;
    float m_wvp[16];
    float m_color[4];
    char m_padB4[0x0c];
    m3g::Transform m_transform;
    int m_scissorX;
    int m_scissorY;
    int m_scissorW;
    int m_scissorH;
};

extern "C" bool g_invalidProgramState;
void invalidateGraphicsIPhoneShaderState();

void transpose(float* m);

}
