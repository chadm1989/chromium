/*	
    WebHistoryItem.h
    Copyright 2001, 2002, Apple, Inc. All rights reserved.
*/

#import <Cocoa/Cocoa.h>

@interface WebHistoryItem : NSObject
{
    NSURL *_url;
    NSString *_target;
    NSString *_parent;
    NSString *_title;
    NSString *_displayTitle;
    NSImage *_image;
    NSCalendarDate *_lastVisitedDate;
    NSPoint _scrollPoint;
    NSString *anchor;
}

+(WebHistoryItem *)entryWithURL:(NSURL *)url;

- (id)init;
- (id)initWithURL:(NSURL *)url title:(NSString *)title;
- (id)initWithURL:(NSURL *)url title:(NSString *)title image:(NSImage *)image;
- (id)initWithURL:(NSURL *)url target: (NSString *)target parent: (NSString *)parent title:(NSString *)title image:(NSImage *)image;

- (NSDictionary *)dictionaryRepresentation;
- (id)initFromDictionaryRepresentation:(NSDictionary *)dict;

- (NSURL *)url;
- (NSString *)target;
- (NSString *)parent;
- (NSString *)title;
- (NSString *)displayTitle;
- (NSImage *)image;
- (NSCalendarDate *)lastVisitedDate;

- (void)setURL:(NSURL *)url;
- (void)setTarget:(NSString *)target;
- (void)setParent:(NSString *)parent;
- (void)setTitle:(NSString *)title;
- (void)setDisplayTitle:(NSString *)displayTitle;
- (void)setImage:(NSImage *)image;
- (void)setLastVisitedDate:(NSCalendarDate *)date;
- (void)setScrollPoint: (NSPoint)p;
- (NSPoint)scrollPoint;
- (unsigned)hash;
- (BOOL)isEqual:(id)anObject;
- (NSString *)anchor;
- (void)setAnchor: (NSString *)anchor;

@end
