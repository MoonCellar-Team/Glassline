#pragma once
#include "sdk/CoreGraphics.h"
typedef signed char BOOL;
typedef unsigned int NSUInteger;
typedef int NSInteger;
typedef unsigned short unichar;
#define YES ((BOOL)1)
#define NO ((BOOL)0)
#define nil ((id)0)
struct NSRange { NSUInteger location,length; };
struct CGAffineTransform { float a,b,c,d,tx,ty; };
struct NSFastEnumerationState { unsigned long state; id* itemsPtr; unsigned long* mutationsPtr; unsigned long extra[5]; };
@class NSString,NSArray,NSDictionary,NSEnumerator,UIColor,UIFont,UIView,UIWindow,NSURL,NSDate,NSDateComponents,NSError;
__attribute__((objc_root_class))
@interface NSObject { Class isa; }
+ (id)alloc; + (id)new; + (Class)class;
- (id)init; - (id)retain; - (oneway void)release; - (id)autorelease; - (id)copy; - (void)dealloc;
- (void)performSelector:(SEL)selector withObject:(id)object afterDelay:(double)delay;
- (NSUInteger)countByEnumeratingWithState:(NSFastEnumerationState*)state objects:(id*)objects count:(NSUInteger)count;
@end
@interface NSString : NSObject
+ (id)stringWithFormat:(NSString*)format,...;
+ (id)stringWithCString:(const char*)text encoding:(NSUInteger)encoding;
- (BOOL)isEqualToString:(NSString*)other; - (BOOL)hasPrefix:(NSString*)prefix;
- (NSInteger)compare:(id)other; - (NSString*)substringWithRange:(NSRange)range;
- (NSString*)stringByAppendingString:(NSString*)other;
- (CGSize)sizeWithFont:(UIFont*)font constrainedToSize:(CGSize)size lineBreakMode:(NSInteger)mode;
@end
@interface NSMutableString : NSString
+ (id)stringWithCapacity:(NSUInteger)capacity;
- (void)appendFormat:(NSString*)format,...;
@end
@interface NSData : NSObject
- (NSUInteger)length; - (const void*)bytes;
@end
@interface NSArray : NSObject
- (NSUInteger)count; - (id)objectAtIndex:(NSUInteger)index; - (NSEnumerator*)objectEnumerator;
@end
@interface NSMutableArray : NSArray
- (void)addObject:(id)object;
@end
@interface NSDictionary : NSObject
+ (id)dictionaryWithObjectsAndKeys:(id)first,...;
- (id)objectForKey:(id)key; - (id)valueForKey:(NSString*)key;
@end
@interface NSMutableDictionary : NSDictionary
- (void)setObject:(id)object forKey:(id)key; - (void)removeAllObjects;
@end
@interface NSEnumerator : NSObject
- (id)nextObject;
@end
@interface NSScanner : NSObject
+ (id)scannerWithString:(NSString*)string;
- (BOOL)scanUpToString:(NSString*)string intoString:(NSString**)out;
- (NSUInteger)scanLocation; - (void)setScanLocation:(NSUInteger)location;
- (BOOL)isAtEnd; - (BOOL)scanHexInt:(unsigned*)value;
@end
@interface NSAutoreleasePool : NSObject @end
@interface NSTimer : NSObject
+ (id)timerWithTimeInterval:(double)interval target:(id)target selector:(SEL)selector userInfo:(id)info repeats:(BOOL)repeats;
- (void)invalidate;
@end
@interface NSRunLoop : NSObject
+ (id)mainRunLoop; - (void)addTimer:(NSTimer*)timer forMode:(NSString*)mode;
@end
@interface NSBundle : NSObject
+ (id)mainBundle; - (id)objectForInfoDictionaryKey:(NSString*)key;
@end
@interface NSNumber : NSObject
+ (id)numberWithBool:(BOOL)value;
@end
@interface NSNotificationCenter : NSObject
+ (id)defaultCenter;
- (void)addObserver:(id)observer selector:(SEL)selector name:(NSString*)name object:(id)object;
- (void)removeObserver:(id)observer name:(NSString*)name object:(id)object;
@end
@interface NSDate : NSObject
+ (id)date; - (NSInteger)compare:(NSDate*)other;
@end
@interface NSCalendar : NSObject
- (id)initWithCalendarIdentifier:(NSString*)identifier; - (NSDate*)dateFromComponents:(NSDateComponents*)components;
@end
@interface NSDateComponents : NSObject
- (void)setYear:(NSInteger)year; - (void)setMonth:(NSInteger)month; - (void)setDay:(NSInteger)day;
@end
@interface UIColor : NSObject
+ (id)lightGrayColor; + (id)blackColor; - (void*)CGColor;
@end
@interface UIFont : NSObject @end
@interface UIView : NSObject { unsigned char _sdkUIView[44]; }
- (id)initWithFrame:(CGRect)frame; - (id)initWithCoder:(id)coder;
- (CGRect)bounds; - (void)setBounds:(CGRect)bounds; - (void)setFrame:(CGRect)frame;
- (void)setCenter:(CGPoint)point; - (void)setTransform:(CGAffineTransform)transform;
- (void)setOpaque:(BOOL)value; - (void)setHidden:(BOOL)value;
- (void)setBackgroundColor:(UIColor*)color; - (void)setMultipleTouchEnabled:(BOOL)value; - (void)setExclusiveTouch:(BOOL)value;
- (id)layer; - (UIView*)superview; - (NSArray*)subviews;
- (void)addSubview:(UIView*)view; - (void)bringSubviewToFront:(UIView*)view; - (void)removeFromSuperview;
- (void)drawRect:(CGRect)rect; - (void)layoutSubviews;
+ (void)beginAnimations:(NSString*)name context:(void*)context; + (void)setAnimationDuration:(double)duration;
+ (void)setAnimationDelegate:(id)delegate; + (void)setAnimationDidStopSelector:(SEL)selector; + (void)commitAnimations;
@end
@interface UIViewController : NSObject { unsigned char _sdkUIViewController[145]; }
- (UIView*)view; - (void)setView:(UIView*)view;
- (void)loadView; - (void)viewDidLoad; - (void)viewDidUnload; - (void)didReceiveMemoryWarning;
- (void)presentModalViewController:(id)controller animated:(BOOL)animated; - (void)dismissModalViewControllerAnimated:(BOOL)animated;
@end
@interface UIWindow : UIView
- (void)makeKeyAndVisible;
@end
@interface UIScreen : NSObject
+ (id)mainScreen; - (CGRect)bounds;
@end
@interface UIApplication : NSObject
+ (id)sharedApplication; - (UIWindow*)keyWindow;
- (void)setStatusBarHidden:(BOOL)hidden; - (void)setStatusBarHidden:(BOOL)hidden withAnimation:(NSInteger)animation;
- (void)setStatusBarOrientation:(NSInteger)orientation animated:(BOOL)animated; - (void)setStatusBarStyle:(NSInteger)style animated:(BOOL)animated;
- (void)setIdleTimerDisabled:(BOOL)disabled;
@end
@interface UILabel : UIView { unsigned char _sdkUILabel[54]; }
- (NSString*)text; - (UIFont*)font; - (UIColor*)textColor;
@end
@interface UITextField : UIView
- (void)setDelegate:(id)delegate; - (void)addTarget:(id)target action:(SEL)action forControlEvents:(NSUInteger)events;
- (BOOL)becomeFirstResponder; - (BOOL)resignFirstResponder;
- (void)setBorderStyle:(NSInteger)style; - (void)setTextColor:(UIColor*)color;
- (void)setAdjustsFontSizeToFitWidth:(BOOL)value; - (void)setClearButtonMode:(NSInteger)mode;
- (void)setKeyboardType:(NSInteger)type; - (void)setReturnKeyType:(NSInteger)type;
- (void)setEnablesReturnKeyAutomatically:(BOOL)value; - (void)setPlaceholder:(NSString*)text;
@end
@interface UIImagePickerController : UIViewController { unsigned char _sdkUIImagePicker[123]; }
- (void)setDelegate:(id)delegate;
@end
@interface MPMediaPickerController : UIViewController
- (id)initWithMediaTypes:(NSUInteger)types; - (void)setDelegate:(id)delegate; - (void)setAllowsPickingMultipleItems:(BOOL)value;
@end
@interface MPMusicPlayerController : NSObject
+ (id)iPodMusicPlayer; - (void)setQueueWithItemCollection:(id)collection; - (void)play;
@end
@interface MPMoviePlayerController : NSObject
- (id)initWithContentURL:(NSURL*)url; - (void)setScalingMode:(NSInteger)mode; - (void)play; - (void)stop;
@end
@interface UIDevice : NSObject @end
@interface UIAccelerometer : NSObject
+ (id)sharedAccelerometer; - (void)setDelegate:(id)delegate; - (void)setUpdateInterval:(double)interval;
@end
@interface UIAcceleration : NSObject
- (double)x; - (double)y; - (double)z;
@end
@interface UITouch : NSObject
- (CGPoint)locationInView:(UIView*)view; - (CGPoint)previousLocationInView:(UIView*)view; - (NSUInteger)tapCount;
@end
@interface NSSet : NSObject @end
@interface CAEAGLLayer : NSObject
- (void)setBounds:(CGRect)bounds; - (void)setOpaque:(BOOL)opaque; - (void)setDrawableProperties:(NSDictionary*)properties;
@end
enum {
    kEAGLRenderingAPIOpenGLES1 = 1,
    kEAGLRenderingAPIOpenGLES2 = 2
};
enum {
    GL_DEPTH_COMPONENT16 = 0x81a5,
    GL_COLOR_ATTACHMENT0 = 0x8ce0,
    GL_DEPTH_ATTACHMENT = 0x8d00,
    GL_FRAMEBUFFER_COMPLETE = 0x8cd5,
    GL_FRAMEBUFFER = 0x8d40,
    GL_RENDERBUFFER = 0x8d41,
    GL_RENDERBUFFER_WIDTH = 0x8d42,
    GL_RENDERBUFFER_HEIGHT = 0x8d43
};
@interface EAGLContext : NSObject
- (id)initWithAPI:(NSUInteger)api; + (BOOL)setCurrentContext:(EAGLContext*)context; + (id)currentContext;
- (BOOL)renderbufferStorage:(NSUInteger)target fromDrawable:(id)drawable; - (BOOL)presentRenderbuffer:(NSUInteger)target;
@end
extern NSString *NSDefaultRunLoopMode,*UIImagePickerControllerOriginalImage,*MPMoviePlayerPlaybackDidFinishNotification;
extern NSString *UIApplicationLaunchOptionsRemoteNotificationKey,*NSGregorianCalendar;
extern NSString *kEAGLColorFormatRGB565,*kEAGLColorFormatRGBA8,*kEAGLDrawablePropertyRetainedBacking,*kEAGLDrawablePropertyColorFormat;
extern "C" {
void objc_setProperty(id,SEL,long,id,BOOL,BOOL);
void NSLog(NSString*,...);
int UIApplicationMain(int,char**,NSString*,NSString*);
int AudioSessionInitialize(void* runLoop,void* runLoopMode,void (*listener)(void*,unsigned long),void* client);
int AudioSessionAddPropertyListener(unsigned long property,void (*listener)(void*,unsigned long,unsigned long,void const*),void* client);
CGContext* UIGraphicsGetCurrentContext();
void CGContextSetStrokeColorWithColor(CGContext*,void*); void CGContextSetLineWidth(CGContext*,float);
void CGContextAddLineToPoint(CGContext*,float,float); void CGContextStrokePath(CGContext*);
CGAffineTransform CGAffineTransformMakeTranslation(float,float); CGAffineTransform CGAffineTransformMakeRotation(float);
CGAffineTransform CGAffineTransformRotate(CGAffineTransform,float);
CGRect CGRectMake(float,float,float,float); CGPoint CGPointMake(float,float);
}
