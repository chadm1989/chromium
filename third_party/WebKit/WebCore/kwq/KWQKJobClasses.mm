/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <kwqdebug.h>

#include <qstring.h>
#include <jobclasses.h>

#include <Foundation/Foundation.h>
#include <WCURICache.h>

static const QString DEFAULT_ERROR_TEXT = "DEFAULT_ERROR_TEXT";

namespace KIO {

// class Job ===================================================================

Job::~Job()
{
}


int Job::error()
{
    _logNeverImplemented();
    return 0;
}


const QString &Job::errorText()
{
    _logNotYetImplemented();
    return DEFAULT_ERROR_TEXT;
}


QString Job::errorString()
{
    _logNotYetImplemented();
    return QString();
}


void Job::kill(bool quietly=TRUE)
{
    _logNotYetImplemented();
}


// class SimpleJob =============================================================

SimpleJob::~SimpleJob()
{
    _logNotYetImplemented();
}


// class TransferJobPrivate ====================================================

class TransferJobPrivate
{
friend class TransferJob;
public:

    TransferJobPrivate(TransferJob *parent, KURL &kurl) {
        metaData = [[NSMutableDictionary alloc] initWithCapacity:17];

        // FIXME: create NSURL for now, later KURL and NSURL should play better together
        NSString *string = [NSString stringWithCString:kurl.url().latin1()];
        url = [[NSURL URLWithString:string] retain];
    }

    ~TransferJobPrivate() {
        [metaData autorelease];
        [url autorelease];
    }

private:
    TransferJob *parent;
    NSMutableDictionary *metaData;
    NSURL *url;
    id requestor;
};

// class TransferJob ===========================================================

TransferJob::TransferJob(const KURL &url, bool reload=false, bool showProgressInfo=true)
{
    _url = url;
    _reload = reload;
    _showProgressInfo = showProgressInfo;
    _status = 0;

    d = new TransferJobPrivate(this, _url);
}


TransferJob::~TransferJob()
{
    delete d;
}

bool TransferJob::isErrorPage() const
{
    return (_status != 0);
}

int TransferJob::error()
{
    return _status;
}

QString TransferJob::queryMetaData(const QString &key)
{
    NSString *_key;
    NSString *_value;
    
    _key = QSTRING_TO_NSSTRING(key);
    _value = [d->metaData objectForKey:_key]; 
    if (!_value) {
        return QString::null;
    }
    else {
        return NSSTRING_TO_QSTRING(_value);
    }
}
 
void TransferJob::addMetaData(const QString &key, const QString &value)
{
    NSString *_key = QSTRING_TO_NSSTRING(key);
    NSString *_value = QSTRING_TO_NSSTRING(value);
    [d->metaData setObject:_value forKey:_key];
}

void TransferJob::kill(bool quietly=TRUE)
{
    [WCGetDefaultURICache() cancelRequestWithURL:d->url requestor:d->requestor];
}

void TransferJob::begin(id requestor, void *userData)
{
    //FIXME: load uri
    d->requestor = requestor;
    [WCGetDefaultURICache() requestWithURL:d->url requestor:requestor userData:userData];
}

} // namespace KIO
