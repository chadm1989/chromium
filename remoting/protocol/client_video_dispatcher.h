// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_CLIENT_VIDEO_DISPATCHER_H_
#define REMOTING_PROTOCOL_CLIENT_VIDEO_DISPATCHER_H_

#include "base/compiler_specific.h"
#include "remoting/proto/video.pb.h"
#include "remoting/protocol/channel_dispatcher_base.h"
#include "remoting/protocol/protobuf_message_parser.h"

namespace remoting {
namespace protocol {

class VideoStub;

class ClientVideoDispatcher : public ChannelDispatcherBase {
 public:
  explicit ClientVideoDispatcher(VideoStub* video_stub);
  ~ClientVideoDispatcher() override;

 private:
  ProtobufMessageParser<VideoPacket> parser_;

  DISALLOW_COPY_AND_ASSIGN(ClientVideoDispatcher);
};

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_CLIENT_VIDEO_DISPATCHER_H_
