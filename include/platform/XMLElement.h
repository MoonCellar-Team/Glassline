#pragma once
#include "platform/AppleSDK.h"

@interface XMLElement : NSObject {
@public
    XMLElement* m_parent;
    NSString* m_name;
    NSDictionary* m_attributes;
    NSMutableDictionary* m_children;
    NSString* m_text;
}
- (id)init:(NSString*)name;
- (XMLElement*)parent; - (void)setParent:(XMLElement*)parent;
- (NSString*)name; - (void)setName:(NSString*)name;
- (NSString*)text; - (void)setText:(NSString*)text;
- (NSDictionary*)attributes; - (void)setAttributes:(NSDictionary*)attributes;
- (NSMutableDictionary*)children; - (void)setChildren:(NSMutableDictionary*)children;
- (void)addText:(NSString*)text; - (void)addAttributes:(NSDictionary*)attributes;
- (void)addChild:(NSString*)name child:(XMLElement*)child;
- (void)print:(NSString*)prefix;
@end
