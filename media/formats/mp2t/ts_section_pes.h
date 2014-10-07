// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_FORMATS_MP2T_TS_SECTION_PES_H_
#define MEDIA_FORMATS_MP2T_TS_SECTION_PES_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "media/base/byte_queue.h"
#include "media/formats/mp2t/ts_section.h"

namespace media {
namespace mp2t {

class EsParser;
class TimestampUnroller;

class TsSectionPes : public TsSection {
 public:
  TsSectionPes(scoped_ptr<EsParser> es_parser,
               TimestampUnroller* timestamp_unroller);
  virtual ~TsSectionPes();

  // TsSection implementation.
  virtual bool Parse(bool payload_unit_start_indicator,
                     const uint8* buf, int size) override;
  virtual void Flush() override;
  virtual void Reset() override;

 private:
  // Emit a reassembled PES packet.
  // Return true if successful.
  // |emit_for_unknown_size| is used to force emission for PES packets
  // whose size is unknown.
  bool Emit(bool emit_for_unknown_size);

  // Parse a PES packet, return true if successful.
  bool ParseInternal(const uint8* raw_pes, int raw_pes_size);

  void ResetPesState();

  // Bytes of the current PES.
  ByteQueue pes_byte_queue_;

  // ES parser.
  scoped_ptr<EsParser> es_parser_;

  // Do not start parsing before getting a unit start indicator.
  bool wait_for_pusi_;

  // Used to unroll PTS and DTS.
  TimestampUnroller* const timestamp_unroller_;

  DISALLOW_COPY_AND_ASSIGN(TsSectionPes);
};

}  // namespace mp2t
}  // namespace media

#endif

