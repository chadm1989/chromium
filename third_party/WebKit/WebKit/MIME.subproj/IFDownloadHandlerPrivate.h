//
//  IFDownloadHandlerPrivate.h
//  WebKit
//
//  Created by Chris Blumenberg on Thu Apr 11 2002.
//  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <WebFoundation/WebFoundation.h>
#import <WebKit/IFDownloadHandler.h>
#import <WebKit/IFMIMEHandler.h>

@interface IFDownloadHandlerPrivate : NSObject {
    IFMIMEHandler *mimeHandler;
    IFURLHandle *urlHandle;
    NSString *path;
    BOOL openAfterDownload;
}

- (void) _setMIMEHandler:(IFMIMEHandler *) mHandler;
- (void) _setURLHandle:(IFURLHandle *)uHandle;
- (NSURL *) _url;
- (IFMIMEHandler *) _mimeHandler;
- (void) _cancelDownload;
- (void) _storeAtPath:(NSString *)newPath;
- (void) _finishedDownload;
- (void) _setOpenAfterDownload:(BOOL)open;

@end

@interface IFDownloadHandler (IFPrivate)
- _initWithURLHandle:(IFURLHandle *)uHandle mimeHandler:(IFMIMEHandler *)mHandler;
- (void) _receivedData:(NSData *)data;
- (void) _finishedDownload;

@end