#pragma once
#include "platform/AppleSDK.h"
@interface NSString (HTMLEntities)
+ (NSString*)stringWithHTMLDecodedString:(NSString*)string;
@end
