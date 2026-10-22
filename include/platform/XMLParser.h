#pragma once
#include "platform/AppleSDK.h"
#include "platform/XMLElement.h"

@interface XMLParser : NSObject {
@public
    NSMutableDictionary* m_dictionary;
    XMLElement* m_currentElement;
}
- (XMLElement*)currentElement; - (NSMutableDictionary*)dict;
- (void)setCurrentElement:(XMLElement*)element; - (void)reset;
- (void)parser:(id)parser didStartElement:(NSString*)name namespaceURI:(NSString*)uri qualifiedName:(NSString*)qualified attributes:(NSDictionary*)attributes;
- (void)parser:(id)parser didEndElement:(NSString*)name namespaceURI:(NSString*)uri qualifiedName:(NSString*)qualified;
- (void)parser:(id)parser foundCharacters:(NSString*)text;
@end
