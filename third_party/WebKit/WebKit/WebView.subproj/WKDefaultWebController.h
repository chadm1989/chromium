/*	
    WKDefaultWebController.mm
	Copyright 2001, 2002 Apple, Inc. All rights reserved.
*/
#import <WebKit/WKWebController.h>


/*
*/
@interface WKDefaultWebController : NSObject <WKWebController>
{
@private
    id _controllerPrivate;
}


- initWithView: (WKWebView *)view dataSource: (WKWebDataSource *)dataSource;

- (void)setDirectsAllLinksToSystemBrowser: (BOOL)flag;
- (BOOL)directsAllLinksToSystemBrowser;

// Sets the mainView and the mainDataSource.
- (void)setView: (WKWebView *)view andDataSource: (WKWebDataSource *)dataSource;

// Find the view for the specified data source.
- (WKWebView *)viewForDataSource: (WKWebDataSource *)dataSource;

// Find the data source for the specified view.
- (WKWebDataSource *)dataSourceForView: (WKWebView *)view;

- (WKWebView *)mainView;

- (void)setMainDataSource: (WKWebDataSource *)dataSource;
- (WKWebDataSource *)mainDataSource;

- (void)createViewForDataSource: (WKWebDataSource *)dataSource inFrameNamed: (NSString *)name;

- (void)createViewForDataSource: (WKWebDataSource *)dataSource inIFrame: (id)iFrameIdentifier;

@end