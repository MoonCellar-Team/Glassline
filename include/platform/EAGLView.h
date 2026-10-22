#pragma once
#include "platform/AppleSDK.h"

namespace midp { class Display; class GLES; }

@interface EAGLView : UIView {
@public
    midp::Display* m_display;
    int width;
    int height;
    EAGLContext* context;
    int glesMajorVersion;
    int glesMinorVersion;
    BOOL m_contextValid;
    unsigned viewRenderbuffer;
    unsigned viewFramebuffer;
    unsigned depthRenderbuffer;
    midp::GLES* m_gl;
}
- (void)destroyFramebuffer;
- (int)getGLESMajorVersion;
- (int)getGLESMinorVersion;
- (midp::GLES*)getGL;
- (BOOL)isValid;
- (int)getWidth;
- (int)getHeight;
- (void)layoutSubviews;
- (void)draw;
- (void)bind;
- (void)flush;
+ (Class)layerClass;
- (id)initContextWithFrame:(CGRect)frame;
- (id)initWithFrame:(CGRect)frame;
- (void)touchesCancelled:(NSSet*)touches withEvent:(id)event;
- (void)touchesEnded:(NSSet*)touches withEvent:(id)event;
- (void)touchesMoved:(NSSet*)touches withEvent:(id)event;
- (void)touchesBegan:(NSSet*)touches withEvent:(id)event;
- (void)dealloc;
- (BOOL)createFramebuffer;
- (id)initWithCoder:(id)coder;
@end
