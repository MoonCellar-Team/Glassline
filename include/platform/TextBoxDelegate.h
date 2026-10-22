#pragma once
#include "platform/AppleSDK.h"

namespace midp { class TextBoxImplementation; }

@interface TextBoxDelegate : NSObject {
@public
    midp::TextBoxImplementation* m_implementation;
    BOOL m_animating;
}
- (id)initWithImplementation:(midp::TextBoxImplementation*)implementation;
- (void)endEditing;
- (BOOL)textFieldShouldReturn:(id)field;
- (void)animationDidStop:(id)animation finished:(id)finished context:(void*)context;
- (void)viewWillDisappear:(id)view;
- (void)viewWillAppear:(id)view;
- (void)remove:(id)view;
- (void)removeActual:(id)view;
- (void)hideNotify:(id)field;
- (void)showNotify:(id)field;
- (void)createTextField;
@end
