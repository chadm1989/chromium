/*
    WebIconDatabase.h
    Copyright 2001, 2002, Apple, Inc. All rights reserved.

    Public header file.
*/

#import <Cocoa/Cocoa.h>

// Sent whenever a site icon has changed. The object of the notification is the icon database.
// The userInfo contains the site URL who's icon has changed.
// It can be accessed with the key WebIconNotificationUserInfoSiteURLKey.
extern NSString *WebIconDidChangeNotification;

extern NSString *WebIconNotificationUserInfoSiteURLKey;

extern NSSize WebIconSmallSize;  // 16 x 16
extern NSSize WebIconMediumSize; // 32 x 32
extern NSSize WebIconLargeSize;  // 128 x 128

@class WebIconDatabasePrivate;

/*!
    @class WebIconDatabase
    @discussion Features:
        - memory cache icons at different sizes
        - disk storage
        - icon update notification
        - user/client icon customization
        
        Uses:
        - WebIconLoader to cache icon images
        - UI elements to retrieve icons that represent site URLs.
        - Save icons to disk for later use.
 
    Every icon in the database has a retain count.  If an icon has a retain count greater than 0, it will be written to disk for later use. If an icon's retain count equals zero it will be removed from disk.  The retain count is not persistent across launches. If the WebKit client wishes to retain an icon it should retain the icon once for every launch.  This is best done at initialization time before the database begins removing icons.  To make sure that the database does not remove unretained icons prematurely, call delayDatabaseCleanup until all desired icons are retained.  Once all are retained, call allowDatabaseCleanup.
    
    Note that an icon can be retained after the database clean-up has begun. This just has to be done before the icon is removed. Icons are removed from the database whenever new icons are added to it.
    
    Retention methods can be called for icons that are not yet in the database.
*/
@interface WebIconDatabase : NSObject {

@private
    WebIconDatabasePrivate *_private;
}


/*!
    @method sharedIconDatabase
    @abstract Returns a shared instance of the icon database
*/
+ (WebIconDatabase *)sharedIconDatabase;

/*!
    @method iconForSiteURL:withSize:
    @discussion Returns an icon for a web site URL from memory or disk. nil if none is found.
    Usually called by a UI element to determine if a site URL has an associated icon.
    Also usually called by the observer of WebIconChangedNotification after the notification is sent.
    @param siteURL
    @param size
*/
- (NSImage *)iconForSiteURL:(NSURL *)siteURL withSize:(NSSize)size;

/*!
    @method defaultIconWithSize:
    @param size
*/
- (NSImage *)defaultIconWithSize:(NSSize)size;

/*!
    @method setIcon:forHost:
    @abstract Customize the site icon for all web sites with the given host name.
    @param icon
    @param host
*/
- (void)setIcon:(NSImage *)icon forHost:(NSString *)host;

/*!
    @method setIcon:forSiteURL:
    @abstract Customize the site icon for a specific web page.
    @param icon
    @param siteURL
*/
- (void)setIcon:(NSImage *)icon forSiteURL:(NSURL *)siteURL;

/*!
    @method retainIconForSiteURL:
    @abstract Increments the retain count of the icon.
    @param siteURL
*/
- (void)retainIconForSiteURL:(NSURL *)siteURL;

/*!
    @method releaseIconForSiteURL:
    @abstract Decrements the retain count of the icon.
    @param siteURL
*/
- (void)releaseIconForSiteURL:(NSURL *)siteURL;

/*!
    @method delayDatabaseCleanup:
    @discussion Only effective if called before the database begins removing icons.
    delayDatabaseCleanUp increments an internal counter that when 0 begins the database clean-up.
    The counter equals 0 at initialization.
*/
- (void)delayDatabaseCleanup;

/*!
    @method allowDatabaseCleanup:
    @discussion Informs the database that it now can begin removing icons.
    allowDatabaseCleanup decrements an internal counter that when 0 begins the database clean-up.
    The counter equals 0 at initialization.
*/
- (void)allowDatabaseCleanup;

@end
