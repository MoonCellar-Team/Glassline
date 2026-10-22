#pragma once

#include "game/SystemStringRenderer.h"

class SystemStringRendererIPhone : public SystemStringRenderer {
public:
    enum RenderingStyle {
        STYLE_FILL = 1,
        STYLE_STROKE = 2,
        STYLE_FILL_STROKE = 3,
    };

    SystemStringRendererIPhone(midp::Font* font, midp::Font* fallbackFont);
    virtual ~SystemStringRendererIPhone();
    virtual void drawString(midp::Graphics* g, midp::String* str, int x, int y, int anchor);
    virtual void drawString(midp::Graphics* g, midp::StringBuffer* str, int x, int y, int anchor);
    virtual void drawSubstring(midp::Graphics* g, midp::String* str, int offset, int length, int x, int y,
                               int anchor);
    virtual int getHeight() const;
    virtual int stringWidth(midp::String* str) const;
    virtual int substringWidth(midp::String* str, int offset, int length);
    virtual void getStringTexturePadding(int& left, int& right, int& top, int& bottom);
    virtual void setIPhoneRenderingStyle(int style);
    virtual void useIPhoneFontDropShadow(bool enable);
    virtual void setIPhoneDropShadowParameters(int x, int y, int blur);
    virtual void setIPhoneDropShadowColor(int argb);
    void setStrokeColor(int color);

    int m_renderingStyle;
    int m_shadowX;
    int m_shadowY;
    int m_shadowBlur;
    int m_strokeColor;
    int m_shadowColor;
    bool m_dropShadow;
    midp::Font* m_fallbackFont;
};
