//
//  WKPluginView.m
//  
//
//  Created by Chris Blumenberg on Thu Dec 13 2001.
//  Copyright (c) 2001 __MyCompanyName__. All rights reserved.
//

#import "WKPluginView.h"
#include <WCURICacheData.h>
#include <WCURICache.h>

@implementation WKPluginView

- initWithFrame: (NSRect) r widget: (QWidget *)w plugin: (WKPlugin *)plug url: (NSString *)location mime:(NSString *)mimeType
{
    NPError npErr;
    char cMime[200];
    NPSavedData saved;
    
    [super initWithFrame: r];

    instance = &instanceStruct;
    stream = &streamStruct;
    streamOffset = 0;

    mime = mimeType;
    url = location;
    plugin = plug;
    NPP_New = 		[plugin NPP_New]; // copy function pointers
    NPP_Destroy = 	[plugin NPP_Destroy];
    NPP_SetWindow = 	[plugin NPP_SetWindow];
    NPP_NewStream = 	[plugin NPP_NewStream];
    NPP_WriteReady = 	[plugin NPP_WriteReady];
    NPP_Write = 	[plugin NPP_Write];
    NPP_DestroyStream = [plugin NPP_DestroyStream];
    NPP_HandleEvent = 	[plugin NPP_HandleEvent];

    [mime getCString:cMime];
    npErr = NPP_New(cMime, instance, NP_EMBED, 0, NULL, NULL, &saved); //need to pass parameters to plug-in
    KWQDebug("NPP_New: %d\n", npErr);
    transferred = FALSE;
    return self;
}

- (void)drawRect:(NSRect)rect {
    NPError npErr;
    char cMime[200], cURL[800];
    uint16 stype;
    id <WCURICache> cache;
    NSRect frame;
    
    frame = [self frame];
    
    nPort.port = [self qdPort];
    nPort.portx = (int32)rect.origin.x;
    nPort.porty = (int32)rect.origin.y;
    window.window = &nPort;
    window.x = 0; 
    window.y = 0;
    //window.x = (uint32)frame.origin.x; //top-left corner of the plug-in relative to page
    //window.y = (uint32)frame.origin.y;
    window.width = (uint32)frame.size.width;
    window.height = (uint32)frame.size.height;
    window.clipRect.top = (uint16)rect.origin.y; // clip rect
    window.clipRect.left = (uint16)rect.origin.x;
    window.clipRect.bottom = (uint16)rect.size.height;
    window.clipRect.right = (uint16)rect.size.width;
    window.type = NPWindowTypeDrawable;
    
    //SetPort(nPort.port);
    //LineTo((int)frame.size.width, (int)frame.size.height);
    //MoveTo(0,0);
    
    npErr = NPP_SetWindow(instance, &window);
    KWQDebug("NPP_SetWindow: %d rect.size.height=%d rect.size.width=%d port=%d rect.origin.x=%f rect.origin.y=%f\n", npErr, (int)rect.size.height, (int)rect.size.width, (int)nPort.port, rect.origin.x, rect.origin.y);
    KWQDebug("frame.size.height=%d frame.size.width=%d frame.origin.x=%f frame.origin.y=%f\n", (int)frame.size.height, (int)frame.size.width, frame.origin.x, frame.origin.y);
    
    if(!transferred){
        [url getCString:cURL];
        stream->url = cURL;
        stream->end = 0;
        stream->lastmodified = 0;
        stream->notifyData = NULL;
        [mime getCString:cMime];
        
        npErr = NPP_NewStream(instance, cMime, stream, TRUE, &stype);
        KWQDebug("NPP_NewStream: %d\n", npErr);
        
        cache = WCGetDefaultURICache();
        if(stype == NP_NORMAL){
            KWQDebug("Stream type: NP_NORMAL\n");
            [cache requestWithString:url requestor:self userData:nil];
        }else if(stype == NP_ASFILEONLY){
            KWQDebug("Stream type: NP_ASFILEONLY\n");
        }else if(stype == NP_ASFILE){
            KWQDebug("Stream type: NP_ASFILE\n");
        }else if(stype == NP_SEEK){
            KWQDebug("Stream type: NP_SEEK\n");
        }
        transferred = TRUE;
    }
}


-(void)cacheDataAvailable:(NSNotification *)notification
{
    id <WCURICacheData> data;
    int32 bytes;
    
    data = [notification object];
    bytes = NPP_WriteReady(instance, stream);
    KWQDebug("NPP_WriteReady bytes=%d\n", (int)bytes);
    
    bytes = NPP_Write(instance, stream, streamOffset, [data cacheDataSize], [data cacheData]);
    KWQDebug("NPP_Write bytes=%d\n", (int)bytes);
    streamOffset += [data cacheDataSize];
}

-(void)cacheFinished:(NSNotification *)notification
{
    NPError npErr;
    streamOffset = 0;
    npErr = NPP_DestroyStream(instance, stream, NPRES_DONE);
    KWQDebug("NPP_DestroyStream: %d\n", npErr);
}

-(BOOL)acceptsFirstResponder
{
    return true;
}

-(void)sendNullEvent
{
    EventRecord event;
    
    event.what = 0;
    KWQDebug("NPP_HandleEvent: %d\n", NPP_HandleEvent(instance, &event));
}

-(void)dealloc
{
    NPError npErr;
    
    npErr = NPP_Destroy(instance, NULL);
    KWQDebug("NPP_Destroy: %d\n", npErr);
    [super dealloc];
}

@end
