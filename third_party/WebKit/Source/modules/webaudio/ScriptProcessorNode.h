/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ScriptProcessorNode_h
#define ScriptProcessorNode_h

#include "base/gtest_prod_util.h"
#include "bindings/core/v8/ActiveScriptWrappable.h"
#include "modules/webaudio/AudioNode.h"
#include "platform/audio/AudioBus.h"
#include "wtf/Forward.h"
#include "wtf/PassRefPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

namespace blink {

class BaseAudioContext;
class AudioBuffer;

// ScriptProcessorNode is an AudioNode which allows for arbitrary synthesis or processing directly using JavaScript.
// The API allows for a variable number of inputs and outputs, although it must have at least one input or output.
// This basic implementation supports no more than one input and output.
// The "onaudioprocess" attribute is an event listener which will get called periodically with an AudioProcessingEvent which has
// AudioBuffers for each input and output.

class ScriptProcessorHandler final : public AudioHandler {
public:
    static PassRefPtr<ScriptProcessorHandler> create(AudioNode&, float sampleRate, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfOutputChannels);
    ~ScriptProcessorHandler() override;

    // AudioHandler
    void process(size_t framesToProcess) override;
    void initialize() override;

    size_t bufferSize() const { return m_bufferSize; }

    void setChannelCount(unsigned long, ExceptionState&) override;
    void setChannelCountMode(const String&, ExceptionState&) override;

    virtual unsigned numberOfOutputChannels() const { return m_numberOfOutputChannels; }

private:
    ScriptProcessorHandler(AudioNode&, float sampleRate, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfOutputChannels);
    double tailTime() const override;
    double latencyTime() const override;

    void fireProcessEvent(unsigned);

    // Double buffering
    unsigned doubleBufferIndex() const { return m_doubleBufferIndex; }
    void swapBuffers() { m_doubleBufferIndex = 1 - m_doubleBufferIndex; }
    unsigned m_doubleBufferIndex;

    // These Persistent don't make reference cycles including the owner
    // ScriptProcessorNode.
    PersistentHeapVector<Member<AudioBuffer>> m_inputBuffers;
    PersistentHeapVector<Member<AudioBuffer>> m_outputBuffers;

    size_t m_bufferSize;
    unsigned m_bufferReadWriteIndex;

    unsigned m_numberOfInputChannels;
    unsigned m_numberOfOutputChannels;

    RefPtr<AudioBus> m_internalInputBus;
    // Synchronize process() with fireProcessEvent().
    mutable Mutex m_processEventLock;

    FRIEND_TEST_ALL_PREFIXES(ScriptProcessorNodeTest, BufferLifetime);
};

class ScriptProcessorNode final : public AudioNode, public ActiveScriptWrappable {
    DEFINE_WRAPPERTYPEINFO();
    USING_GARBAGE_COLLECTED_MIXIN(ScriptProcessorNode);
public:
    // bufferSize must be one of the following values: 256, 512, 1024, 2048,
    // 4096, 8192, 16384.
    // This value controls how frequently the onaudioprocess event handler is
    // called and how many sample-frames need to be processed each call.
    // Lower numbers for bufferSize will result in a lower (better)
    // latency. Higher numbers will be necessary to avoid audio breakup and
    // glitches.
    // The value chosen must carefully balance between latency and audio quality.
    static ScriptProcessorNode* create(BaseAudioContext&, ExceptionState&);
    static ScriptProcessorNode* create(BaseAudioContext&, size_t bufferSize, ExceptionState&);
    static ScriptProcessorNode* create(BaseAudioContext&, size_t bufferSize, unsigned numberOfInputChannels, ExceptionState&);
    static ScriptProcessorNode* create(BaseAudioContext&, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, ExceptionState&);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(audioprocess);
    size_t bufferSize() const;

    // ActiveScriptWrappable
    bool hasPendingActivity() const final;

    DEFINE_INLINE_VIRTUAL_TRACE() { AudioNode::trace(visitor); }

private:
    ScriptProcessorNode(BaseAudioContext&, float sampleRate, size_t bufferSize, unsigned numberOfInputChannels, unsigned numberOfOutputChannels);
};

} // namespace blink

#endif // ScriptProcessorNode_h
