//
//  WebDownloadHandler.h
//  WebKit
//
//  Created by Chris Blumenberg on Thu Apr 11 2002.
//  Copyright (c) 2002 Apple computer, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

@class WebDataSource;
@class WebError;
@class WebResourceResponse;

@interface WebDownloadHandler : NSObject
{
    WebDataSource *dataSource;
    NSFileHandle *fileHandle;
}

- initWithDataSource:(WebDataSource *)dSource;
- (WebError *)receivedResponse:(WebResourceResponse *)response; 
- (void)receivedData:(NSData *)data;
- (WebError *)finishedLoading;
- (WebError *)cancel;
@end
