// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "example/http_server/http_server.h"

#include <utility>
#include <iostream>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/at_exit.h"
#include "base/command_line.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/log/net_log_source.h"
#include "net/server/http_server_request_info.h"
#include "net/socket/tcp_server_socket.h"

const int kBufferSize = 100 * 1024 * 1024;  // 100 MB

TestHttpServer::TestHttpServer()
    : thread_("ServerThread"),
      all_closed_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                        base::WaitableEvent::InitialState::SIGNALED),
      request_action_(kAccept),
      message_action_(kEchoMessage) {}

TestHttpServer::~TestHttpServer() {
}

bool TestHttpServer::Start() {
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  bool thread_started = thread_.StartWithOptions(options);
  if (!thread_started)
    return false;
  bool success;
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&TestHttpServer::StartOnServerThread,
                            base::Unretained(this), &success, &event));
  event.Wait();
  return success;
}

void TestHttpServer::Stop() {
  if (!thread_.IsRunning())
    return;
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&TestHttpServer::StopOnServerThread,
                            base::Unretained(this), &event));
  event.Wait();
  thread_.Stop();
}

bool TestHttpServer::WaitForConnectionsToClose() {
  return all_closed_event_.TimedWait(base::TimeDelta::FromSeconds(10));
}

void TestHttpServer::SetRequestAction(WebSocketRequestAction action) {
  base::AutoLock lock(action_lock_);
  request_action_ = action;
}

void TestHttpServer::SetMessageAction(WebSocketMessageAction action) {
  base::AutoLock lock(action_lock_);
  message_action_ = action;
}

void TestHttpServer::SetMessageCallback(const base::Closure& callback) {
  base::AutoLock lock(action_lock_);
  message_callback_ = callback;
}

GURL TestHttpServer::web_socket_url() const {
  base::AutoLock lock(url_lock_);
  return web_socket_url_;
}

void TestHttpServer::OnConnect(int connection_id) {
  server_->SetSendBufferSize(connection_id, kBufferSize);
  server_->SetReceiveBufferSize(connection_id, kBufferSize);

  std::cout << "HTTP Server OnConnect:" << connection_id << std::endl;
}

void TestHttpServer::OnHttpRequest(
  int connection_id,
  const net::HttpServerRequestInfo& info) {
  std::cout << "HTTP Server OnHttpRequest:" << connection_id << std::endl;
  server_->Send200(connection_id, "hello", "text/plain");
}

void TestHttpServer::OnClose(int connection_id) {
  all_closed_event_.Signal();
}

void TestHttpServer::StartOnServerThread(bool* success,
                                         base::WaitableEvent* event) {
  std::unique_ptr<net::ServerSocket> server_socket(
      new net::TCPServerSocket(NULL, net::NetLogSource()));
  server_socket->ListenWithAddressAndPort("127.0.0.1", 0, 1);
  server_.reset(new net::HttpServer(std::move(server_socket), this));

  net::IPEndPoint address;
  int error = server_->GetLocalAddress(&address);

  if (error == net::OK) {
    std::cout << "HTTP Server Started on port:" << address.port() << std::endl;
    base::AutoLock lock(url_lock_);
    web_socket_url_ = GURL(base::StringPrintf("ws://127.0.0.1:%d",
                                              address.port()));
  } else {
    server_.reset(NULL);
  }
  *success = (server_.get() != nullptr);
  event->Signal();
}

void TestHttpServer::StopOnServerThread(base::WaitableEvent* event) {
  server_.reset(NULL);
  event->Signal();
}

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  TestHttpServer* server = new TestHttpServer();
  server->Start();

  while (1) {
    Sleep(1000);
  }

  return 0;
}