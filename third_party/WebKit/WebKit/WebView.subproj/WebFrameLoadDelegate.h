/*	
        WebLocationChangeHandler.h
	Copyright 2001, 2002, Apple, Inc. All rights reserved.

        Public header file.
*/

#import <Foundation/Foundation.h>

@class WebError;
@class WebDataSource;
@class WebFrame;

/*!
    @protcol WebLocationChangeHandler
    @abstract A controller's WebLocationChangeHandler track changes it's frame's location. 
*/
@protocol WebLocationChangeHandler <NSObject>

/*!
    @method locationChangeStartedForDataSource:
    @param dataSource
*/
- (void)locationChangeStartedForDataSource:(WebDataSource *)dataSource;

/*!
    @method locationChangeCommittedForDataSource:
    @param dataSource
*/
- (void)locationChangeCommittedForDataSource:(WebDataSource *)dataSource;

/*!
    @method locationChangeDone:forDataSource
    @param error
    @param dataSource
*/
- (void)locationChangeDone:(WebError *)error forDataSource:(WebDataSource *)dataSource;

/*!
    @method receivedPageTitle:forDataSource:
    @param title
    @param dataSource
*/
- (void)receivedPageTitle:(NSString *)title forDataSource:(WebDataSource *)dataSource;

/*!
    @method receivedPageIcon:forDataSource:
    @param image
    @param dataSource
*/
- (void)receivedPageIcon:(NSImage *)image forDataSource:(WebDataSource *)dataSource;

/*!
    @method serverRedirectTo:forDataSource:
    @param URL
    @param dataSource
*/
- (void)serverRedirectTo:(NSURL *)URL forDataSource:(WebDataSource *)dataSource;

/*!
    @method clientRedirectTo:delay:fireDate:forFrame:
    @param URL
    @param seconds
    @param date
    @param frame
*/
- (void)clientRedirectTo:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date forFrame:(WebFrame *)frame;

/*!
    @method clientRedirectCancelledForFrame:
    @param frame
*/
- (void)clientRedirectCancelledForFrame:(WebFrame *)frame;

@end

/*!
    @class WebLocationChangeHandler
    @discussion The WebLocationChangeHandler class responds to all WebLocationChangeHandler protocol
    methods by doing nothing. It's provided for the convenience of clients who only want
    to implement some of the above methods and ignore others.
*/
@interface WebLocationChangeHandler : NSObject <WebLocationChangeHandler>
{
}
@end
