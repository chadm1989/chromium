// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ipc/ipc_channel_mojo.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "base/pickle.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_io_thread.h"
#include "base/test/test_timeouts.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_mojo_handle_attachment.h"
#include "ipc/ipc_mojo_message_helper.h"
#include "ipc/ipc_mojo_param_traits.h"
#include "ipc/ipc_test.mojom.h"
#include "ipc/ipc_test_base.h"
#include "ipc/ipc_test_channel_listener.h"
#include "mojo/edk/test/mojo_test_base.h"
#include "mojo/edk/test/multiprocess_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX)
#include "base/file_descriptor_posix.h"
#include "ipc/ipc_platform_file_attachment_posix.h"
#endif

#define DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(client_name, test_base)       \
  class client_name##_MainFixture : public test_base {                    \
   public:                                                                \
    void Main();                                                          \
  };                                                                      \
  MULTIPROCESS_TEST_MAIN_WITH_SETUP(                                      \
      client_name##TestChildMain,                                         \
      ::mojo::edk::test::MultiprocessTestHelper::ChildSetup) {            \
    CHECK(!mojo::edk::test::MultiprocessTestHelper::primordial_pipe_token \
               .empty());                                                 \
    client_name##_MainFixture test;                                       \
    test.Init(mojo::edk::CreateChildMessagePipe(                          \
        mojo::edk::test::MultiprocessTestHelper::primordial_pipe_token)); \
    test.Main();                                                          \
    return (::testing::Test::HasFatalFailure() ||                         \
            ::testing::Test::HasNonfatalFailure())                        \
               ? 1                                                        \
               : 0;                                                       \
  }                                                                       \
  void client_name##_MainFixture::Main()

namespace {

void SendString(IPC::Sender* sender, const std::string& str) {
  IPC::Message* message = new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
  message->WriteString(str);
  ASSERT_TRUE(sender->Send(message));
}

class ListenerThatExpectsOK : public IPC::Listener {
 public:
  ListenerThatExpectsOK() : received_ok_(false) {}

  ~ListenerThatExpectsOK() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    std::string should_be_ok;
    EXPECT_TRUE(iter.ReadString(&should_be_ok));
    EXPECT_EQ(should_be_ok, "OK");
    received_ok_ = true;
    base::MessageLoop::current()->QuitWhenIdle();
    return true;
  }

  void OnChannelError() override {
    // The connection should be healthy while the listener is waiting
    // message.  An error can occur after that because the peer
    // process dies.
    DCHECK(received_ok_);
  }

  static void SendOK(IPC::Sender* sender) { SendString(sender, "OK"); }

 private:
  bool received_ok_;
};

class ChannelClient {
 public:
  void Init(mojo::ScopedMessagePipeHandle handle) {
    handle_ = std::move(handle);
  }

  void Connect(IPC::Listener* listener) {
    channel_ = IPC::ChannelMojo::Create(std::move(handle_),
                                        IPC::Channel::MODE_CLIENT, listener);
    CHECK(channel_->Connect());
  }

  void Close() {
    channel_->Close();

    base::RunLoop run_loop;
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  run_loop.QuitClosure());
    run_loop.Run();
  }

  IPC::ChannelMojo* channel() const { return channel_.get(); }

 private:
  base::MessageLoopForIO main_message_loop_;
  mojo::ScopedMessagePipeHandle handle_;
  std::unique_ptr<IPC::ChannelMojo> channel_;
};

class IPCChannelMojoTestBase : public testing::Test {
 public:
  void InitWithMojo(const std::string& test_client_name) {
    handle_ = helper_.StartChild(test_client_name);
  }

  bool WaitForClientShutdown() { return helper_.WaitForChildTestShutdown(); }

 protected:
  mojo::ScopedMessagePipeHandle TakeHandle() { return std::move(handle_); }

 private:
  mojo::ScopedMessagePipeHandle handle_;
  mojo::edk::test::MultiprocessTestHelper helper_;
};

class IPCChannelMojoTest : public IPCChannelMojoTestBase {
 public:
  void TearDown() override { base::RunLoop().RunUntilIdle(); }

  void CreateChannel(IPC::Listener* listener) {
    channel_ = IPC::ChannelMojo::Create(
        TakeHandle(), IPC::Channel::MODE_SERVER, listener);
  }

  bool ConnectChannel() { return channel_->Connect(); }

  void DestroyChannel() { channel_.reset(); }

  IPC::Sender* sender() { return channel(); }
  IPC::Channel* channel() { return channel_.get(); }

 private:
  base::MessageLoop message_loop_;
  std::unique_ptr<IPC::Channel> channel_;
};

class TestChannelListenerWithExtraExpectations
    : public IPC::TestChannelListener {
 public:
  TestChannelListenerWithExtraExpectations() : is_connected_called_(false) {}

  void OnChannelConnected(int32_t peer_pid) override {
    IPC::TestChannelListener::OnChannelConnected(peer_pid);
    EXPECT_TRUE(base::kNullProcessId != peer_pid);
    is_connected_called_ = true;
  }

  bool is_connected_called() const { return is_connected_called_; }

 private:
  bool is_connected_called_;
};

TEST_F(IPCChannelMojoTest, ConnectedFromClient) {
  InitWithMojo("IPCChannelMojoTestClient");

  // Set up IPC channel and start client.
  TestChannelListenerWithExtraExpectations listener;
  CreateChannel(&listener);
  listener.Init(sender());
  ASSERT_TRUE(ConnectChannel());

  IPC::TestChannelListener::SendOneMessage(sender(), "hello from parent");

  base::RunLoop().Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  EXPECT_TRUE(listener.is_connected_called());
  EXPECT_TRUE(listener.HasSentAll());

  DestroyChannel();
}

// A long running process that connects to us
DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestClient, ChannelClient) {
  TestChannelListenerWithExtraExpectations listener;
  Connect(&listener);
  listener.Init(channel());

  IPC::TestChannelListener::SendOneMessage(channel(), "hello from child");
  base::RunLoop().Run();
  EXPECT_TRUE(listener.is_connected_called());
  EXPECT_TRUE(listener.HasSentAll());

  Close();
}

class ListenerExpectingErrors : public IPC::Listener {
 public:
  ListenerExpectingErrors() : has_error_(false) {}

  void OnChannelConnected(int32_t peer_pid) override {
    base::MessageLoop::current()->QuitWhenIdle();
  }

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelError() override {
    has_error_ = true;
    base::MessageLoop::current()->QuitWhenIdle();
  }

  bool has_error() const { return has_error_; }

 private:
  bool has_error_;
};

class ListenerThatQuits : public IPC::Listener {
 public:
  ListenerThatQuits() {}

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelConnected(int32_t peer_pid) override {
    base::MessageLoop::current()->QuitWhenIdle();
  }
};

// A long running process that connects to us.
DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoErraticTestClient,
                                    ChannelClient) {
  ListenerThatQuits listener;
  Connect(&listener);

  base::RunLoop().Run();

  Close();
}

TEST_F(IPCChannelMojoTest, SendFailWithPendingMessages) {
  InitWithMojo("IPCChannelMojoErraticTestClient");

  // Set up IPC channel and start client.
  ListenerExpectingErrors listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  // This matches a value in mojo/edk/system/constants.h
  const int kMaxMessageNumBytes = 4 * 1024 * 1024;
  std::string overly_large_data(kMaxMessageNumBytes, '*');
  // This messages are queued as pending.
  for (size_t i = 0; i < 10; ++i) {
    IPC::TestChannelListener::SendOneMessage(sender(),
                                             overly_large_data.c_str());
  }

  base::RunLoop().Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  EXPECT_TRUE(listener.has_error());

  DestroyChannel();
}

struct TestingMessagePipe {
  TestingMessagePipe() {
    EXPECT_EQ(MOJO_RESULT_OK, mojo::CreateMessagePipe(nullptr, &self, &peer));
  }

  mojo::ScopedMessagePipeHandle self;
  mojo::ScopedMessagePipeHandle peer;
};

class HandleSendingHelper {
 public:
  static std::string GetSendingFileContent() { return "Hello"; }

  static void WritePipe(IPC::Message* message, TestingMessagePipe* pipe) {
    std::string content = HandleSendingHelper::GetSendingFileContent();
    EXPECT_EQ(MOJO_RESULT_OK,
              mojo::WriteMessageRaw(pipe->self.get(), &content[0],
                                    static_cast<uint32_t>(content.size()),
                                    nullptr, 0, 0));
    EXPECT_TRUE(IPC::MojoMessageHelper::WriteMessagePipeTo(
        message, std::move(pipe->peer)));
  }

  static void WritePipeThenSend(IPC::Sender* sender, TestingMessagePipe* pipe) {
    IPC::Message* message =
        new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
    WritePipe(message, pipe);
    ASSERT_TRUE(sender->Send(message));
  }

  static void ReadReceivedPipe(const IPC::Message& message,
                               base::PickleIterator* iter) {
    mojo::ScopedMessagePipeHandle pipe;
    EXPECT_TRUE(
        IPC::MojoMessageHelper::ReadMessagePipeFrom(&message, iter, &pipe));
    std::string content(GetSendingFileContent().size(), ' ');

    uint32_t num_bytes = static_cast<uint32_t>(content.size());
    ASSERT_EQ(MOJO_RESULT_OK,
              mojo::Wait(pipe.get(), MOJO_HANDLE_SIGNAL_READABLE,
                         MOJO_DEADLINE_INDEFINITE, nullptr));
    EXPECT_EQ(MOJO_RESULT_OK,
              mojo::ReadMessageRaw(pipe.get(), &content[0], &num_bytes, nullptr,
                                   nullptr, 0));
    EXPECT_EQ(content, GetSendingFileContent());
  }

#if defined(OS_POSIX)
  static base::FilePath GetSendingFilePath(const base::FilePath& dir_path) {
    return dir_path.Append("ListenerThatExpectsFile.txt");
  }

  static void WriteFile(IPC::Message* message, base::File& file) {
    std::string content = GetSendingFileContent();
    file.WriteAtCurrentPos(content.data(), content.size());
    file.Flush();
    message->WriteAttachment(new IPC::internal::PlatformFileAttachment(
        base::ScopedFD(file.TakePlatformFile())));
  }

  static void WriteFileThenSend(IPC::Sender* sender, base::File& file) {
    IPC::Message* message =
        new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
    WriteFile(message, file);
    ASSERT_TRUE(sender->Send(message));
  }

  static void WriteFileAndPipeThenSend(IPC::Sender* sender,
                                       base::File& file,
                                       TestingMessagePipe* pipe) {
    IPC::Message* message =
        new IPC::Message(0, 2, IPC::Message::PRIORITY_NORMAL);
    WriteFile(message, file);
    WritePipe(message, pipe);
    ASSERT_TRUE(sender->Send(message));
  }

  static void ReadReceivedFile(const IPC::Message& message,
                               base::PickleIterator* iter) {
    base::ScopedFD fd;
    scoped_refptr<base::Pickle::Attachment> attachment;
    EXPECT_TRUE(message.ReadAttachment(iter, &attachment));
    EXPECT_EQ(IPC::MessageAttachment::TYPE_PLATFORM_FILE,
              static_cast<IPC::MessageAttachment*>(attachment.get())
                  ->GetType());
    base::File file(static_cast<IPC::MessageAttachment*>(attachment.get())
                        ->TakePlatformFile());
    std::string content(GetSendingFileContent().size(), ' ');
    file.Read(0, &content[0], content.size());
    EXPECT_EQ(content, GetSendingFileContent());
  }
#endif
};

class ListenerThatExpectsMessagePipe : public IPC::Listener {
 public:
  ListenerThatExpectsMessagePipe() : sender_(NULL) {}

  ~ListenerThatExpectsMessagePipe() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    HandleSendingHelper::ReadReceivedPipe(message, &iter);
    ListenerThatExpectsOK::SendOK(sender_);
    return true;
  }

  void OnChannelError() override {
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void set_sender(IPC::Sender* sender) { sender_ = sender; }

 private:
  IPC::Sender* sender_;
};

TEST_F(IPCChannelMojoTest, SendMessagePipe) {
  InitWithMojo("IPCChannelMojoTestSendMessagePipeClient");

  ListenerThatExpectsOK listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  TestingMessagePipe pipe;
  HandleSendingHelper::WritePipeThenSend(channel(), &pipe);

  base::RunLoop().Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendMessagePipeClient,
                                    ChannelClient) {
  ListenerThatExpectsMessagePipe listener;
  Connect(&listener);
  listener.set_sender(channel());

  base::RunLoop().Run();

  Close();
}

void ReadOK(mojo::MessagePipeHandle pipe) {
  std::string should_be_ok("xx");
  uint32_t num_bytes = static_cast<uint32_t>(should_be_ok.size());
  CHECK_EQ(MOJO_RESULT_OK, mojo::Wait(pipe, MOJO_HANDLE_SIGNAL_READABLE,
                                      MOJO_DEADLINE_INDEFINITE, nullptr));
  CHECK_EQ(MOJO_RESULT_OK,
           mojo::ReadMessageRaw(pipe, &should_be_ok[0], &num_bytes, nullptr,
                                nullptr, 0));
  EXPECT_EQ(should_be_ok, std::string("OK"));
}

void WriteOK(mojo::MessagePipeHandle pipe) {
  std::string ok("OK");
  CHECK_EQ(MOJO_RESULT_OK,
           mojo::WriteMessageRaw(pipe, &ok[0], static_cast<uint32_t>(ok.size()),
                                 nullptr, 0, 0));
}

class ListenerThatExpectsMessagePipeUsingParamTrait : public IPC::Listener {
 public:
  explicit ListenerThatExpectsMessagePipeUsingParamTrait(bool receiving_valid)
      : sender_(NULL), receiving_valid_(receiving_valid) {}

  ~ListenerThatExpectsMessagePipeUsingParamTrait() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    mojo::MessagePipeHandle handle;
    EXPECT_TRUE(IPC::ParamTraits<mojo::MessagePipeHandle>::Read(&message, &iter,
                                                                &handle));
    EXPECT_EQ(handle.is_valid(), receiving_valid_);
    if (receiving_valid_) {
      ReadOK(handle);
      MojoClose(handle.value());
    }

    ListenerThatExpectsOK::SendOK(sender_);
    return true;
  }

  void OnChannelError() override {
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void set_sender(IPC::Sender* sender) { sender_ = sender; }

 private:
  IPC::Sender* sender_;
  bool receiving_valid_;
};

class ParamTraitMessagePipeClient : public ChannelClient {
 public:
  void RunTest(bool receiving_valid_handle) {
    ListenerThatExpectsMessagePipeUsingParamTrait listener(
        receiving_valid_handle);
    Connect(&listener);
    listener.set_sender(channel());

    base::RunLoop().Run();

    Close();
  }
};

TEST_F(IPCChannelMojoTest, ParamTraitValidMessagePipe) {
  InitWithMojo("ParamTraitValidMessagePipeClient");

  ListenerThatExpectsOK listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  TestingMessagePipe pipe;

  std::unique_ptr<IPC::Message> message(new IPC::Message());
  IPC::ParamTraits<mojo::MessagePipeHandle>::Write(message.get(),
                                                   pipe.peer.release());
  WriteOK(pipe.self.get());

  channel()->Send(message.release());
  base::RunLoop().Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(ParamTraitValidMessagePipeClient,
                                    ParamTraitMessagePipeClient) {
  RunTest(true);
}

TEST_F(IPCChannelMojoTest, ParamTraitInvalidMessagePipe) {
  InitWithMojo("ParamTraitInvalidMessagePipeClient");

  ListenerThatExpectsOK listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  mojo::MessagePipeHandle invalid_handle;
  std::unique_ptr<IPC::Message> message(new IPC::Message());
  IPC::ParamTraits<mojo::MessagePipeHandle>::Write(message.get(),
                                                   invalid_handle);

  channel()->Send(message.release());
  base::RunLoop().Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(ParamTraitInvalidMessagePipeClient,
                                    ParamTraitMessagePipeClient) {
  RunTest(false);
}

TEST_F(IPCChannelMojoTest, SendFailAfterClose) {
  InitWithMojo("IPCChannelMojoTestSendOkClient");

  ListenerThatExpectsOK listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  base::RunLoop().Run();
  channel()->Close();
  ASSERT_FALSE(channel()->Send(new IPC::Message()));

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

class ListenerSendingOneOk : public IPC::Listener {
 public:
  ListenerSendingOneOk() {}

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelConnected(int32_t peer_pid) override {
    ListenerThatExpectsOK::SendOK(sender_);
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void set_sender(IPC::Sender* sender) { sender_ = sender; }

 private:
  IPC::Sender* sender_;
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendOkClient,
                                    ChannelClient) {
  ListenerSendingOneOk listener;
  Connect(&listener);
  listener.set_sender(channel());

  base::RunLoop().Run();

  Close();
}

class ListenerWithSimpleAssociatedInterface
    : public IPC::Listener,
      public IPC::mojom::SimpleTestDriver {
 public:
  static const int kNumMessages;

  ListenerWithSimpleAssociatedInterface() : binding_(this) {}

  ~ListenerWithSimpleAssociatedInterface() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    std::string should_be_expected;
    EXPECT_TRUE(iter.ReadString(&should_be_expected));
    EXPECT_EQ(should_be_expected, next_expected_string_);
    num_messages_received_++;
    return true;
  }

  void OnChannelError() override {
    DCHECK(received_quit_);
  }

  void RegisterInterfaceFactory(IPC::Channel* channel) {
    channel->GetAssociatedInterfaceSupport()->AddAssociatedInterface(
        base::Bind(&ListenerWithSimpleAssociatedInterface::BindRequest,
                   base::Unretained(this)));
  }

 private:
  // IPC::mojom::SimpleTestDriver:
  void ExpectString(const mojo::String& str) override {
    next_expected_string_ = str;
  }

  void RequestQuit(const RequestQuitCallback& callback) override {
    EXPECT_EQ(kNumMessages, num_messages_received_);
    received_quit_ = true;
    callback.Run();
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void BindRequest(IPC::mojom::SimpleTestDriverAssociatedRequest request) {
    DCHECK(!binding_.is_bound());
    binding_.Bind(std::move(request));
  }

  std::string next_expected_string_;
  int num_messages_received_ = 0;
  bool received_quit_ = false;

  mojo::AssociatedBinding<IPC::mojom::SimpleTestDriver> binding_;
};

const int ListenerWithSimpleAssociatedInterface::kNumMessages = 1000;

class ListenerSendingAssociatedMessages : public IPC::Listener {
 public:
  ListenerSendingAssociatedMessages() {}

  bool OnMessageReceived(const IPC::Message& message) override { return true; }

  void OnChannelConnected(int32_t peer_pid) override {
    DCHECK(channel_);
    channel_->GetAssociatedInterfaceSupport()->GetRemoteAssociatedInterface(
        &driver_);

    // Send a bunch of interleaved messages, alternating between the associated
    // interface and a legacy IPC::Message.
    for (int i = 0; i < ListenerWithSimpleAssociatedInterface::kNumMessages;
         ++i) {
      std::string str = base::StringPrintf("Hello! %d", i);
      driver_->ExpectString(str);
      SendString(channel_, str);
    }
    driver_->RequestQuit(base::Bind(&OnQuitAck));
  }

  void set_channel(IPC::Channel* channel) { channel_ = channel; }

 private:
  static void OnQuitAck() { base::MessageLoop::current()->QuitWhenIdle(); }

  IPC::Channel* channel_ = nullptr;
  IPC::mojom::SimpleTestDriverAssociatedPtr driver_;
};

TEST_F(IPCChannelMojoTest, SimpleAssociatedInterface) {
  InitWithMojo("SimpleAssociatedInterfaceClient");

  ListenerWithSimpleAssociatedInterface listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  listener.RegisterInterfaceFactory(channel());

  base::RunLoop().Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(SimpleAssociatedInterfaceClient,
                                    ChannelClient) {
  ListenerSendingAssociatedMessages listener;
  Connect(&listener);
  listener.set_channel(channel());

  base::RunLoop().Run();

  Close();
}

class ChannelProxyRunner {
 public:
  ChannelProxyRunner(std::unique_ptr<IPC::ChannelFactory> channel_factory)
      : channel_factory_(std::move(channel_factory)),
        io_thread_("ChannelProxyRunner IO thread") {
  }

  void CreateProxy(IPC::Listener* listener) {
    io_thread_.StartWithOptions(
        base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
    proxy_.reset(new IPC::ChannelProxy(listener, io_thread_.task_runner()));
  }
  void RunProxy() { proxy_->Init(std::move(channel_factory_), true); }

  IPC::ChannelProxy* proxy() { return proxy_.get(); }

 private:
  std::unique_ptr<IPC::ChannelFactory> channel_factory_;

  base::Thread io_thread_;
  std::unique_ptr<IPC::ChannelProxy> proxy_;

  DISALLOW_COPY_AND_ASSIGN(ChannelProxyRunner);
};

class IPCChannelProxyMojoTest : public IPCChannelMojoTestBase {
 public:
  void InitWithMojo(const std::string& client_name) {
    IPCChannelMojoTestBase::InitWithMojo(client_name);
    runner_.reset(new ChannelProxyRunner(
        IPC::ChannelMojo::CreateServerFactory(TakeHandle())));
  }
  void CreateProxy(IPC::Listener* listener) { runner_->CreateProxy(listener); }
  void RunProxy() { runner_->RunProxy(); }

  IPC::ChannelProxy* proxy() { return runner_->proxy(); }

 private:
  base::MessageLoop message_loop_;
  std::unique_ptr<ChannelProxyRunner> runner_;
};

class ListenerWithSimpleProxyAssociatedInterface
    : public IPC::Listener,
      public IPC::mojom::SimpleTestDriver {
 public:
  static const int kNumMessages;

  ListenerWithSimpleProxyAssociatedInterface() : binding_(this) {}

  ~ListenerWithSimpleProxyAssociatedInterface() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    std::string should_be_expected;
    EXPECT_TRUE(iter.ReadString(&should_be_expected));
    EXPECT_EQ(should_be_expected, next_expected_string_);
    num_messages_received_++;
    return true;
  }

  void OnChannelError() override {
    DCHECK(received_quit_);
  }

  void RegisterInterfaceFactory(IPC::ChannelProxy* proxy) {
    proxy->AddAssociatedInterface(
        base::Bind(&ListenerWithSimpleProxyAssociatedInterface::BindRequest,
                   base::Unretained(this)));
  }

  bool received_all_messages() const {
    return num_messages_received_ == kNumMessages && received_quit_;
  }

 private:
  // IPC::mojom::SimpleTestDriver:
  void ExpectString(const mojo::String& str) override {
    next_expected_string_ = str;
  }

  void RequestQuit(const RequestQuitCallback& callback) override {
    received_quit_ = true;
    callback.Run();
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void BindRequest(IPC::mojom::SimpleTestDriverAssociatedRequest request) {
    DCHECK(!binding_.is_bound());
    binding_.Bind(std::move(request));
  }

  std::string next_expected_string_;
  int num_messages_received_ = 0;
  bool received_quit_ = false;

  mojo::AssociatedBinding<IPC::mojom::SimpleTestDriver> binding_;
};

const int ListenerWithSimpleProxyAssociatedInterface::kNumMessages = 1000;

TEST_F(IPCChannelProxyMojoTest, ProxyThreadAssociatedInterface) {
  InitWithMojo("ProxyThreadAssociatedInterfaceClient");

  ListenerWithSimpleProxyAssociatedInterface listener;
  CreateProxy(&listener);
  listener.RegisterInterfaceFactory(proxy());
  RunProxy();

  base::RunLoop().Run();

  EXPECT_TRUE(WaitForClientShutdown());
  EXPECT_TRUE(listener.received_all_messages());

  base::RunLoop().RunUntilIdle();
}

class ChannelProxyClient {
 public:
  void Init(mojo::ScopedMessagePipeHandle handle) {
    runner_.reset(new ChannelProxyRunner(
        IPC::ChannelMojo::CreateClientFactory(std::move(handle))));
  }
  void CreateProxy(IPC::Listener* listener) { runner_->CreateProxy(listener); }
  void RunProxy() { runner_->RunProxy(); }

  IPC::ChannelProxy* proxy() { return runner_->proxy(); }

 private:
  base::MessageLoop message_loop_;
  std::unique_ptr<ChannelProxyRunner> runner_;
};

class ListenerThatWaitsForConnect : public IPC::Listener {
 public:
  explicit ListenerThatWaitsForConnect(const base::Closure& connect_handler)
      : connect_handler_(connect_handler) {}

  // IPC::Listener
  bool OnMessageReceived(const IPC::Message& message) override { return true; }
  void OnChannelConnected(int32_t) override { connect_handler_.Run(); }

 private:
  base::Closure connect_handler_;
};

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(ProxyThreadAssociatedInterfaceClient,
                                    ChannelProxyClient) {
  base::RunLoop connect_loop;
  ListenerThatWaitsForConnect listener(connect_loop.QuitClosure());
  CreateProxy(&listener);
  RunProxy();
  connect_loop.Run();

  // Send a bunch of interleaved messages, alternating between the associated
  // interface and a legacy IPC::Message.
  IPC::mojom::SimpleTestDriverAssociatedPtr driver;
  proxy()->GetRemoteAssociatedInterface(&driver);
  for (int i = 0; i < ListenerWithSimpleProxyAssociatedInterface::kNumMessages;
       ++i) {
    std::string str = base::StringPrintf("Hello! %d", i);
    driver->ExpectString(str);
    SendString(proxy(), str);
  }
  driver->RequestQuit(base::MessageLoop::QuitWhenIdleClosure());
  base::RunLoop().Run();
}

#if defined(OS_POSIX)

class ListenerThatExpectsFile : public IPC::Listener {
 public:
  ListenerThatExpectsFile() : sender_(NULL) {}

  ~ListenerThatExpectsFile() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    HandleSendingHelper::ReadReceivedFile(message, &iter);
    ListenerThatExpectsOK::SendOK(sender_);
    return true;
  }

  void OnChannelError() override {
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void set_sender(IPC::Sender* sender) { sender_ = sender; }

 private:
  IPC::Sender* sender_;
};

TEST_F(IPCChannelMojoTest, SendPlatformHandle) {
  InitWithMojo("IPCChannelMojoTestSendPlatformHandleClient");

  ListenerThatExpectsOK listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::File file(HandleSendingHelper::GetSendingFilePath(temp_dir.path()),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
                      base::File::FLAG_READ);
  HandleSendingHelper::WriteFileThenSend(channel(), file);
  base::RunLoop().Run();

  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestSendPlatformHandleClient,
                                    ChannelClient) {
  ListenerThatExpectsFile listener;
  Connect(&listener);
  listener.set_sender(channel());

  base::RunLoop().Run();

  Close();
}

class ListenerThatExpectsFileAndPipe : public IPC::Listener {
 public:
  ListenerThatExpectsFileAndPipe() : sender_(NULL) {}

  ~ListenerThatExpectsFileAndPipe() override {}

  bool OnMessageReceived(const IPC::Message& message) override {
    base::PickleIterator iter(message);
    HandleSendingHelper::ReadReceivedFile(message, &iter);
    HandleSendingHelper::ReadReceivedPipe(message, &iter);
    ListenerThatExpectsOK::SendOK(sender_);
    return true;
  }

  void OnChannelError() override {
    base::MessageLoop::current()->QuitWhenIdle();
  }

  void set_sender(IPC::Sender* sender) { sender_ = sender; }

 private:
  IPC::Sender* sender_;
};

TEST_F(IPCChannelMojoTest, SendPlatformHandleAndPipe) {
  InitWithMojo("IPCChannelMojoTestSendPlatformHandleAndPipeClient");

  ListenerThatExpectsOK listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::File file(HandleSendingHelper::GetSendingFilePath(temp_dir.path()),
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE |
                      base::File::FLAG_READ);
  TestingMessagePipe pipe;
  HandleSendingHelper::WriteFileAndPipeThenSend(channel(), file, &pipe);

  base::RunLoop().Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(
    IPCChannelMojoTestSendPlatformHandleAndPipeClient,
    ChannelClient) {
  ListenerThatExpectsFileAndPipe listener;
  Connect(&listener);
  listener.set_sender(channel());

  base::RunLoop().Run();

  Close();
}

#endif  // defined(OS_POSIX)

#if defined(OS_LINUX)

const base::ProcessId kMagicChildId = 54321;

class ListenerThatVerifiesPeerPid : public IPC::Listener {
 public:
  void OnChannelConnected(int32_t peer_pid) override {
    EXPECT_EQ(peer_pid, kMagicChildId);
    base::MessageLoop::current()->QuitWhenIdle();
  }

  bool OnMessageReceived(const IPC::Message& message) override {
    NOTREACHED();
    return true;
  }
};

TEST_F(IPCChannelMojoTest, VerifyGlobalPid) {
  InitWithMojo("IPCChannelMojoTestVerifyGlobalPidClient");

  ListenerThatVerifiesPeerPid listener;
  CreateChannel(&listener);
  ASSERT_TRUE(ConnectChannel());

  base::MessageLoop::current()->Run();
  channel()->Close();

  EXPECT_TRUE(WaitForClientShutdown());
  DestroyChannel();
}

DEFINE_IPC_CHANNEL_MOJO_TEST_CLIENT(IPCChannelMojoTestVerifyGlobalPidClient,
                                    ChannelClient) {
  IPC::Channel::SetGlobalPid(kMagicChildId);
  ListenerThatQuits listener;
  Connect(&listener);

  base::MessageLoop::current()->Run();

  Close();
}

#endif  // OS_LINUX

}  // namespace
