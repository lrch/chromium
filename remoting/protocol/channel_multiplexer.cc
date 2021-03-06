// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/protocol/channel_multiplexer.h"

#include <string.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "net/socket/stream_socket.h"
#include "remoting/protocol/message_serialization.h"

namespace remoting {
namespace protocol {

namespace {
const int kChannelIdUnknown = -1;
const int kMaxPacketSize = 1024;

class PendingPacket {
 public:
  PendingPacket(scoped_ptr<MultiplexPacket> packet,
                const base::Closure& done_task)
      : packet(packet.Pass()),
        done_task(done_task),
        pos(0U) {
  }
  ~PendingPacket() {
    done_task.Run();
  }

  bool is_empty() { return pos >= packet->data().size(); }

  int Read(char* buffer, size_t size) {
    size = std::min(size, packet->data().size() - pos);
    memcpy(buffer, packet->data().data() + pos, size);
    pos += size;
    return size;
  }

 private:
  scoped_ptr<MultiplexPacket> packet;
  base::Closure done_task;
  size_t pos;

  DISALLOW_COPY_AND_ASSIGN(PendingPacket);
};

}  // namespace

const char ChannelMultiplexer::kMuxChannelName[] = "mux";

struct ChannelMultiplexer::PendingChannel {
  PendingChannel(const std::string& name,
                 const ChannelCreatedCallback& callback)
      : name(name), callback(callback) {
  }
  std::string name;
  ChannelCreatedCallback callback;
};

class ChannelMultiplexer::MuxChannel {
 public:
  MuxChannel(ChannelMultiplexer* multiplexer, const std::string& name,
             int send_id);
  ~MuxChannel();

  const std::string& name() { return name_; }
  int receive_id() { return receive_id_; }
  void set_receive_id(int id) { receive_id_ = id; }

  // Called by ChannelMultiplexer.
  scoped_ptr<net::StreamSocket> CreateSocket();
  void OnIncomingPacket(scoped_ptr<MultiplexPacket> packet,
                        const base::Closure& done_task);
  void OnWriteFailed();

  // Called by MuxSocket.
  void OnSocketDestroyed();
  bool DoWrite(scoped_ptr<MultiplexPacket> packet,
               const base::Closure& done_task);
  int DoRead(net::IOBuffer* buffer, int buffer_len);

 private:
  ChannelMultiplexer* multiplexer_;
  std::string name_;
  int send_id_;
  bool id_sent_;
  int receive_id_;
  MuxSocket* socket_;
  std::list<PendingPacket*> pending_packets_;

  DISALLOW_COPY_AND_ASSIGN(MuxChannel);
};

class ChannelMultiplexer::MuxSocket : public net::StreamSocket,
                                      public base::NonThreadSafe,
                                      public base::SupportsWeakPtr<MuxSocket> {
 public:
  MuxSocket(MuxChannel* channel);
  ~MuxSocket() override;

  void OnWriteComplete();
  void OnWriteFailed();
  void OnPacketReceived();

  // net::StreamSocket interface.
  int Read(net::IOBuffer* buffer,
           int buffer_len,
           const net::CompletionCallback& callback) override;
  int Write(net::IOBuffer* buffer,
            int buffer_len,
            const net::CompletionCallback& callback) override;

  int SetReceiveBufferSize(int32 size) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int SetSendBufferSize(int32 size) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int Connect(const net::CompletionCallback& callback) override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  void Disconnect() override { NOTIMPLEMENTED(); }
  bool IsConnected() const override {
    NOTIMPLEMENTED();
    return true;
  }
  bool IsConnectedAndIdle() const override {
    NOTIMPLEMENTED();
    return false;
  }
  int GetPeerAddress(net::IPEndPoint* address) const override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int GetLocalAddress(net::IPEndPoint* address) const override {
    NOTIMPLEMENTED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  const net::BoundNetLog& NetLog() const override {
    NOTIMPLEMENTED();
    return net_log_;
  }
  void SetSubresourceSpeculation() override { NOTIMPLEMENTED(); }
  void SetOmniboxSpeculation() override { NOTIMPLEMENTED(); }
  bool WasEverUsed() const override { return true; }
  bool UsingTCPFastOpen() const override { return false; }
  bool WasNpnNegotiated() const override { return false; }
  net::NextProto GetNegotiatedProtocol() const override {
    return net::kProtoUnknown;
  }
  bool GetSSLInfo(net::SSLInfo* ssl_info) override {
    NOTIMPLEMENTED();
    return false;
  }

 private:
  MuxChannel* channel_;

  net::CompletionCallback read_callback_;
  scoped_refptr<net::IOBuffer> read_buffer_;
  int read_buffer_size_;

  bool write_pending_;
  int write_result_;
  net::CompletionCallback write_callback_;

  net::BoundNetLog net_log_;

  DISALLOW_COPY_AND_ASSIGN(MuxSocket);
};


ChannelMultiplexer::MuxChannel::MuxChannel(
    ChannelMultiplexer* multiplexer,
    const std::string& name,
    int send_id)
    : multiplexer_(multiplexer),
      name_(name),
      send_id_(send_id),
      id_sent_(false),
      receive_id_(kChannelIdUnknown),
      socket_(nullptr) {
}

ChannelMultiplexer::MuxChannel::~MuxChannel() {
  // Socket must be destroyed before the channel.
  DCHECK(!socket_);
  STLDeleteElements(&pending_packets_);
}

scoped_ptr<net::StreamSocket> ChannelMultiplexer::MuxChannel::CreateSocket() {
  DCHECK(!socket_);  // Can't create more than one socket per channel.
  scoped_ptr<MuxSocket> result(new MuxSocket(this));
  socket_ = result.get();
  return result.Pass();
}

void ChannelMultiplexer::MuxChannel::OnIncomingPacket(
    scoped_ptr<MultiplexPacket> packet,
    const base::Closure& done_task) {
  DCHECK_EQ(packet->channel_id(), receive_id_);
  if (packet->data().size() > 0) {
    pending_packets_.push_back(new PendingPacket(packet.Pass(), done_task));
    if (socket_) {
      // Notify the socket that we have more data.
      socket_->OnPacketReceived();
    }
  }
}

void ChannelMultiplexer::MuxChannel::OnWriteFailed() {
  if (socket_)
    socket_->OnWriteFailed();
}

void ChannelMultiplexer::MuxChannel::OnSocketDestroyed() {
  DCHECK(socket_);
  socket_ = nullptr;
}

bool ChannelMultiplexer::MuxChannel::DoWrite(
    scoped_ptr<MultiplexPacket> packet,
    const base::Closure& done_task) {
  packet->set_channel_id(send_id_);
  if (!id_sent_) {
    packet->set_channel_name(name_);
    id_sent_ = true;
  }
  return multiplexer_->DoWrite(packet.Pass(), done_task);
}

int ChannelMultiplexer::MuxChannel::DoRead(net::IOBuffer* buffer,
                                           int buffer_len) {
  int pos = 0;
  while (buffer_len > 0 && !pending_packets_.empty()) {
    DCHECK(!pending_packets_.front()->is_empty());
    int result = pending_packets_.front()->Read(
        buffer->data() + pos, buffer_len);
    DCHECK_LE(result, buffer_len);
    pos += result;
    buffer_len -= pos;
    if (pending_packets_.front()->is_empty()) {
      delete pending_packets_.front();
      pending_packets_.erase(pending_packets_.begin());
    }
  }
  return pos;
}

ChannelMultiplexer::MuxSocket::MuxSocket(MuxChannel* channel)
    : channel_(channel),
      read_buffer_size_(0),
      write_pending_(false),
      write_result_(0) {
}

ChannelMultiplexer::MuxSocket::~MuxSocket() {
  channel_->OnSocketDestroyed();
}

int ChannelMultiplexer::MuxSocket::Read(
    net::IOBuffer* buffer, int buffer_len,
    const net::CompletionCallback& callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(read_callback_.is_null());

  int result = channel_->DoRead(buffer, buffer_len);
  if (result == 0) {
    read_buffer_ = buffer;
    read_buffer_size_ = buffer_len;
    read_callback_ = callback;
    return net::ERR_IO_PENDING;
  }
  return result;
}

int ChannelMultiplexer::MuxSocket::Write(
    net::IOBuffer* buffer, int buffer_len,
    const net::CompletionCallback& callback) {
  DCHECK(CalledOnValidThread());

  scoped_ptr<MultiplexPacket> packet(new MultiplexPacket());
  size_t size = std::min(kMaxPacketSize, buffer_len);
  packet->mutable_data()->assign(buffer->data(), size);

  write_pending_ = true;
  bool result = channel_->DoWrite(packet.Pass(), base::Bind(
      &ChannelMultiplexer::MuxSocket::OnWriteComplete, AsWeakPtr()));

  if (!result) {
    // Cannot complete the write, e.g. if the connection has been terminated.
    return net::ERR_FAILED;
  }

  // OnWriteComplete() might be called above synchronously.
  if (write_pending_) {
    DCHECK(write_callback_.is_null());
    write_callback_ = callback;
    write_result_ = size;
    return net::ERR_IO_PENDING;
  }

  return size;
}

void ChannelMultiplexer::MuxSocket::OnWriteComplete() {
  write_pending_ = false;
  if (!write_callback_.is_null()) {
    net::CompletionCallback cb;
    std::swap(cb, write_callback_);
    cb.Run(write_result_);
  }
}

void ChannelMultiplexer::MuxSocket::OnWriteFailed() {
  if (!write_callback_.is_null()) {
    net::CompletionCallback cb;
    std::swap(cb, write_callback_);
    cb.Run(net::ERR_FAILED);
  }
}

void ChannelMultiplexer::MuxSocket::OnPacketReceived() {
  if (!read_callback_.is_null()) {
    int result = channel_->DoRead(read_buffer_.get(), read_buffer_size_);
    read_buffer_ = nullptr;
    DCHECK_GT(result, 0);
    net::CompletionCallback cb;
    std::swap(cb, read_callback_);
    cb.Run(result);
  }
}

ChannelMultiplexer::ChannelMultiplexer(StreamChannelFactory* factory,
                                       const std::string& base_channel_name)
    : base_channel_factory_(factory),
      base_channel_name_(base_channel_name),
      next_channel_id_(0),
      parser_(base::Bind(&ChannelMultiplexer::OnIncomingPacket,
                         base::Unretained(this)),
              &reader_),
      weak_factory_(this) {
}

ChannelMultiplexer::~ChannelMultiplexer() {
  DCHECK(pending_channels_.empty());
  STLDeleteValues(&channels_);

  // Cancel creation of the base channel if it hasn't finished.
  if (base_channel_factory_)
    base_channel_factory_->CancelChannelCreation(base_channel_name_);
}

void ChannelMultiplexer::CreateChannel(const std::string& name,
                                       const ChannelCreatedCallback& callback) {
  if (base_channel_.get()) {
    // Already have |base_channel_|. Create new multiplexed channel
    // synchronously.
    callback.Run(GetOrCreateChannel(name)->CreateSocket());
  } else if (!base_channel_.get() && !base_channel_factory_) {
    // Fail synchronously if we failed to create |base_channel_|.
    callback.Run(nullptr);
  } else {
    // Still waiting for the |base_channel_|.
    pending_channels_.push_back(PendingChannel(name, callback));

    // If this is the first multiplexed channel then create the base channel.
    if (pending_channels_.size() == 1U) {
      base_channel_factory_->CreateChannel(
          base_channel_name_,
          base::Bind(&ChannelMultiplexer::OnBaseChannelReady,
                     base::Unretained(this)));
    }
  }
}

void ChannelMultiplexer::CancelChannelCreation(const std::string& name) {
  for (std::list<PendingChannel>::iterator it = pending_channels_.begin();
       it != pending_channels_.end(); ++it) {
    if (it->name == name) {
      pending_channels_.erase(it);
      return;
    }
  }
}

void ChannelMultiplexer::OnBaseChannelReady(
    scoped_ptr<net::StreamSocket> socket) {
  base_channel_factory_ = nullptr;
  base_channel_ = socket.Pass();

  if (base_channel_.get()) {
    // Initialize reader and writer.
    reader_.StartReading(base_channel_.get());
    writer_.Init(base_channel_.get(),
                 base::Bind(&ChannelMultiplexer::OnWriteFailed,
                            base::Unretained(this)));
  }

  DoCreatePendingChannels();
}

void ChannelMultiplexer::DoCreatePendingChannels() {
  if (pending_channels_.empty())
    return;

  // Every time this function is called it connects a single channel and posts a
  // separate task to connect other channels. This is necessary because the
  // callback may destroy the multiplexer or somehow else modify
  // |pending_channels_| list (e.g. call CancelChannelCreation()).
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&ChannelMultiplexer::DoCreatePendingChannels,
                            weak_factory_.GetWeakPtr()));

  PendingChannel c = pending_channels_.front();
  pending_channels_.erase(pending_channels_.begin());
  scoped_ptr<net::StreamSocket> socket;
  if (base_channel_.get())
    socket = GetOrCreateChannel(c.name)->CreateSocket();
  c.callback.Run(socket.Pass());
}

ChannelMultiplexer::MuxChannel* ChannelMultiplexer::GetOrCreateChannel(
    const std::string& name) {
  // Check if we already have a channel with the requested name.
  std::map<std::string, MuxChannel*>::iterator it = channels_.find(name);
  if (it != channels_.end())
    return it->second;

  // Create a new channel if we haven't found existing one.
  MuxChannel* channel = new MuxChannel(this, name, next_channel_id_);
  ++next_channel_id_;
  channels_[channel->name()] = channel;
  return channel;
}


void ChannelMultiplexer::OnWriteFailed(int error) {
  for (std::map<std::string, MuxChannel*>::iterator it = channels_.begin();
       it != channels_.end(); ++it) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&ChannelMultiplexer::NotifyWriteFailed,
                              weak_factory_.GetWeakPtr(), it->second->name()));
  }
}

void ChannelMultiplexer::NotifyWriteFailed(const std::string& name) {
  std::map<std::string, MuxChannel*>::iterator it = channels_.find(name);
  if (it != channels_.end()) {
    it->second->OnWriteFailed();
  }
}

void ChannelMultiplexer::OnIncomingPacket(scoped_ptr<MultiplexPacket> packet,
                                          const base::Closure& done_task) {
  DCHECK(packet->has_channel_id());
  if (!packet->has_channel_id()) {
    LOG(ERROR) << "Received packet without channel_id.";
    done_task.Run();
    return;
  }

  int receive_id = packet->channel_id();
  MuxChannel* channel = nullptr;
  std::map<int, MuxChannel*>::iterator it =
      channels_by_receive_id_.find(receive_id);
  if (it != channels_by_receive_id_.end()) {
    channel = it->second;
  } else {
    // This is a new |channel_id| we haven't seen before. Look it up by name.
    if (!packet->has_channel_name()) {
      LOG(ERROR) << "Received packet with unknown channel_id and "
          "without channel_name.";
      done_task.Run();
      return;
    }
    channel = GetOrCreateChannel(packet->channel_name());
    channel->set_receive_id(receive_id);
    channels_by_receive_id_[receive_id] = channel;
  }

  channel->OnIncomingPacket(packet.Pass(), done_task);
}

bool ChannelMultiplexer::DoWrite(scoped_ptr<MultiplexPacket> packet,
                                 const base::Closure& done_task) {
  return writer_.Write(SerializeAndFrameMessage(*packet), done_task);
}

}  // namespace protocol
}  // namespace remoting
