// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Fix rate send side congestion control, used for testing.

#ifndef NET_QUIC_CONGESTION_CONTROL_FIX_RATE_SENDER_H_
#define NET_QUIC_CONGESTION_CONTROL_FIX_RATE_SENDER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "net/base/net_export.h"
#include "net/quic/quic_clock.h"
#include "net/quic/quic_connection_stats.h"
#include "net/quic/quic_time.h"
#include "net/quic/congestion_control/leaky_bucket.h"
#include "net/quic/congestion_control/paced_sender.h"
#include "net/quic/congestion_control/send_algorithm_interface.h"

namespace net {

class RttStats;

class NET_EXPORT_PRIVATE FixRateSender : public SendAlgorithmInterface {
 public:
  explicit FixRateSender(const RttStats* rtt_stats);
  virtual ~FixRateSender();

  // Start implementation of SendAlgorithmInterface.
  virtual void SetFromConfig(const QuicConfig& config, bool is_server) OVERRIDE;
  virtual void OnIncomingQuicCongestionFeedbackFrame(
      const QuicCongestionFeedbackFrame& feedback,
      QuicTime feedback_receive_time) OVERRIDE;
  virtual void OnPacketAcked(QuicPacketSequenceNumber acked_sequence_number,
                             QuicByteCount acked_bytes) OVERRIDE;
  virtual void OnPacketLost(QuicPacketSequenceNumber sequence_number,
                            QuicTime ack_receive_time) OVERRIDE;
  virtual bool OnPacketSent(
      QuicTime sent_time,
      QuicPacketSequenceNumber sequence_number,
      QuicByteCount bytes,
      HasRetransmittableData has_retransmittable_data) OVERRIDE;
  virtual void OnRetransmissionTimeout(bool packets_retransmitted) OVERRIDE;
  virtual void OnPacketAbandoned(QuicPacketSequenceNumber sequence_number,
                                 QuicByteCount bytes_abandoned) OVERRIDE;
  virtual QuicTime::Delta TimeUntilSend(
      QuicTime now,
      HasRetransmittableData has_retransmittable_data) OVERRIDE;
  virtual QuicBandwidth BandwidthEstimate() const OVERRIDE;
  virtual void OnRttUpdated(QuicPacketSequenceNumber largest_observed) OVERRIDE;
  virtual QuicTime::Delta RetransmissionDelay() const OVERRIDE;
  virtual QuicByteCount GetCongestionWindow() const OVERRIDE;
  // End implementation of SendAlgorithmInterface.

 private:
  QuicByteCount CongestionWindow();

  const RttStats* rtt_stats_;
  QuicBandwidth bitrate_;
  QuicByteCount max_segment_size_;
  LeakyBucket fix_rate_leaky_bucket_;
  PacedSender paced_sender_;
  QuicByteCount data_in_flight_;
  QuicTime::Delta latest_rtt_;

  DISALLOW_COPY_AND_ASSIGN(FixRateSender);
};

}  // namespace net

#endif  // NET_QUIC_CONGESTION_CONTROL_FIX_RATE_SENDER_H_
