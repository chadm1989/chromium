//
//  WebHistoryPrivate.h
//  WebKit
//
//  Created by John Sullivan on Tue Feb 19 2002.
//  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@class WebHistoryItem;

@interface WebHistoryPrivate : NSObject {
@private
    NSMutableDictionary *_entriesByURL;
    NSMutableArray *_datesWithEntries;
    NSMutableArray *_entriesByDate;
    NSString *_file;
}

- (id)initWithFile: (NSString *)file;

- (void)addEntry: (WebHistoryItem *)entry;
- (void)addEntries:(NSArray *)newEntries;
- (BOOL)removeEntry: (WebHistoryItem *)entry;
- (BOOL)removeEntries: (NSArray *)entries;
- (BOOL)removeAllEntries;

- (NSArray *)orderedLastVisitedDays;
- (NSArray *)orderedEntriesLastVisitedOnDay: (NSCalendarDate *)calendarDate;
- (BOOL)containsEntryForURLString: (NSString *)URLString;
- (BOOL)containsURL: (NSURL *)URL;
- (WebHistoryItem *)entryForURL:(NSURL *)URL;

- (NSString *)file;
- (BOOL)loadHistory;
- (BOOL)saveHistory;

@end
