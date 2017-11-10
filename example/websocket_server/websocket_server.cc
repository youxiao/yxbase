// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "example/websocket_server/websocket_server.h"

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

WebSocketServer::WebSocketServer()
    : thread_("WebSocketServerThread"),
      all_closed_event_(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                        base::WaitableEvent::InitialState::SIGNALED),
      request_action_(kAccept),
      message_action_(kEchoMessage) {}

WebSocketServer::~WebSocketServer() {
}

bool WebSocketServer::Start() {
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  bool thread_started = thread_.StartWithOptions(options);
  if (!thread_started)
    return false;
  bool success;
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&WebSocketServer::StartOnServerThread,
                            base::Unretained(this), &success, &event));
  event.Wait();
  return success;
}

void WebSocketServer::Stop() {
  if (!thread_.IsRunning())
    return;
  base::WaitableEvent event(base::WaitableEvent::ResetPolicy::AUTOMATIC,
                            base::WaitableEvent::InitialState::NOT_SIGNALED);
  thread_.task_runner()->PostTask(
      FROM_HERE, base::Bind(&WebSocketServer::StopOnServerThread,
                            base::Unretained(this), &event));
  event.Wait();
  thread_.Stop();
}

bool WebSocketServer::WaitForConnectionsToClose() {
  return all_closed_event_.TimedWait(base::TimeDelta::FromSeconds(10));
}

void WebSocketServer::SetRequestAction(WebSocketRequestAction action) {
  base::AutoLock lock(action_lock_);
  request_action_ = action;
}

void WebSocketServer::SetMessageAction(WebSocketMessageAction action) {
  base::AutoLock lock(action_lock_);
  message_action_ = action;
}

void WebSocketServer::SetMessageCallback(const base::Closure& callback) {
  base::AutoLock lock(action_lock_);
  message_callback_ = callback;
}

GURL WebSocketServer::web_socket_url() const {
  base::AutoLock lock(url_lock_);
  return web_socket_url_;
}

void WebSocketServer::OnConnect(int connection_id) {
  server_->SetSendBufferSize(connection_id, kBufferSize);
  server_->SetReceiveBufferSize(connection_id, kBufferSize);

  std::cout << "WebSocketServer OnConnect:" << connection_id << std::endl;
}

void WebSocketServer::OnWebSocketRequest(
    int connection_id,
    const net::HttpServerRequestInfo& info) {
  WebSocketRequestAction action;
  {
    base::AutoLock lock(action_lock_);
    action = request_action_;
  }
  connections_.insert(connection_id);
  all_closed_event_.Reset();

  std::cout << "WebSocketServer OnWebSocketRequest:" << connection_id << std::endl;

  switch (action) {
    case kAccept:
      server_->AcceptWebSocket(connection_id, info);
      break;
    case kNotFound:
      server_->Send404(connection_id);
      break;
    case kClose:
      server_->Close(connection_id);
      break;
  }
}

void WebSocketServer::OnWebSocketMessage(int connection_id,
                                        const std::string& data) {
  WebSocketMessageAction action;
  base::Closure callback;
  {
    base::AutoLock lock(action_lock_);
    action = message_action_;
    callback = base::ResetAndReturn(&message_callback_);
  }

  std::cout << "WebSocketServer OnWebSocketMessage:" << connection_id << " data:" << data << std::endl;

  if (!callback.is_null())
    callback.Run();
  switch (action) {
    case kEchoMessage:
      server_->SendOverWebSocket(connection_id, data);
      break;
    case kCloseOnMessage:
      server_->Close(connection_id);
      break;
  }
}

void WebSocketServer::OnClose(int connection_id) {
  connections_.erase(connection_id);
  if (connections_.empty())
    all_closed_event_.Signal();
}

void WebSocketServer::StartOnServerThread(bool* success,
                                         base::WaitableEvent* event) {
  std::unique_ptr<net::ServerSocket> server_socket(
      new net::TCPServerSocket(NULL, net::NetLogSource()));
  server_socket->ListenWithAddressAndPort("127.0.0.1", 8888, 1);
  server_.reset(new net::HttpServer(std::move(server_socket), this));

  net::IPEndPoint address;
  int error = server_->GetLocalAddress(&address);

  if (error == net::OK) {
    std::cout << "WebSocketServer Started on port:" << address.port() << std::endl;
    base::AutoLock lock(url_lock_);
    web_socket_url_ = GURL(base::StringPrintf("ws://127.0.0.1:%d",
                                              address.port()));
  } else {
    server_.reset(NULL);
  }
  *success = (server_.get() != nullptr);
  event->Signal();
}

void WebSocketServer::StopOnServerThread(base::WaitableEvent* event) {
  server_.reset(NULL);
  event->Signal();
}

int main(int argc, const char* const* argv) {
  base::CommandLine::Init(argc, argv);
  base::AtExitManager exit_manager;

  WebSocketServer* server = new WebSocketServer();
  server->Start();

  while (1) {
    Sleep(1000);
  }

  return 0;
}