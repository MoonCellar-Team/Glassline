#pragma once
#include "platform/AppleSDK.h"

@interface RuntimeAppDelegate : NSObject
- (void)application:(UIApplication*)application didFailToRegisterForRemoteNotificationsWithError:(NSError*)error;
- (void)applicationDidFinishLaunching:(UIApplication*)application;
- (void)applicationDidEnterBackground:(UIApplication*)application;
- (void)applicationWillEnterForeground:(UIApplication*)application;
- (BOOL)application:(UIApplication*)application handleOpenURL:(NSURL*)url;
- (BOOL)application:(UIApplication*)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions;
- (void)application:(UIApplication*)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)deviceToken;
- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo;
- (void)didFinishLaunching:(UIApplication*)application;
- (void)applicationDidBecomeActive:(UIApplication*)application;
- (void)applicationDidReceiveMemoryWarning:(UIApplication*)application;
- (void)applicationWillTerminate:(UIApplication*)application;
- (void)applicationWillResignActive:(UIApplication*)application;
@end

extern UIWindow* window;
