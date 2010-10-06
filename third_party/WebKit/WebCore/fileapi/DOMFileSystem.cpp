/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DOMFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "DOMFilePath.h"
#include "DirectoryEntry.h"
#include "ErrorCallback.h"
#include "FileEntry.h"
#include "FileSystemCallbacks.h"
#include "FileWriter.h"
#include "FileWriterCallback.h"
#include "MetadataCallback.h"
#include "ScriptExecutionContext.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

DOMFileSystem::DOMFileSystem(ScriptExecutionContext* context, const String& name, PassOwnPtr<AsyncFileSystem> asyncFileSystem)
    : DOMFileSystemBase(name, asyncFileSystem)
    , ActiveDOMObject(context, this)
{
}

PassRefPtr<DirectoryEntry> DOMFileSystem::root()
{
    return DirectoryEntry::create(this, DOMFilePath::root);
}

void DOMFileSystem::stop()
{
    m_asyncFileSystem->stop();
}

bool DOMFileSystem::hasPendingActivity() const
{
    return m_asyncFileSystem->hasPendingActivity();
}

void DOMFileSystem::contextDestroyed()
{
    m_asyncFileSystem->stop();
    ActiveDOMObject::contextDestroyed();
}

void DOMFileSystem::createWriter(const FileEntry* file, PassRefPtr<FileWriterCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ASSERT(file);

    String platformPath = m_asyncFileSystem->virtualToPlatformPath(file->fullPath());

    RefPtr<FileWriter> fileWriter = FileWriter::create(scriptExecutionContext());
    OwnPtr<FileWriterCallbacks> callbacks = FileWriterCallbacks::create(fileWriter, successCallback, errorCallback);
    m_asyncFileSystem->createWriter(fileWriter.get(), platformPath, callbacks.release());
}

} // namespace

#endif // ENABLE(FILE_SYSTEM)
