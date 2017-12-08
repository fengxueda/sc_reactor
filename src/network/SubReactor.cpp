/*
 * SubReactor.cpp
 *
 *  Created on: 2017年12月6日
 *      Author: xueda
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glog/logging.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include "SubReactor.h"
#include "Selector.h"
#include "Session.h"
#include "DataPacket.h"
#include "ServiceMessage.h"
#include "Utils.h"

#define CHECK_STATUS(_level_,_expr_)                                                \
    do {                                                                            \
    if ((_expr_) < 0) {                                                             \
      DLOG(_level_) << "Error code: " << errno << " message : " << strerror(errno); \
      return ;                                                                      \
  }                                                                                 \
} while(0)

namespace network {

SubReactor::SubReactor(SessionManager* session_manager)
    : running_(false),
      reactor_(nullptr) {
  CHECK_NOTNULL(session_manager);
  reactor_ = new std::thread(&SubReactor::Mainloop, this);
  CHECK_NOTNULL(reactor_);
  std::stringstream stream;
  stream << reactor_->get_id();
  reactor_id_ = stream.str();
}

SubReactor::~SubReactor() {
  running_ = false;
  reactor_->join();
  delete reactor_;
  reactor_ = nullptr;
  DLOG(INFO)<< __FUNCTION__;
}

const std::string& SubReactor::reactor_id() const {
  return reactor_id_;
}

void SubReactor::Start() {
  running_ = true;
  cond_var_.notify_one();
}

void SubReactor::Stop() {
  running_ = false;
}

void SubReactor::OnSessionHandler(const std::shared_ptr<Session>& session) {
  OnDataRecv(session);
}

void SubReactor::AddMainloopCallback(const std::function<void()>& callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  mainloop_callbacks_.push_back(callback);
}

void SubReactor::AddPushMessageCallback(
    const std::function<void(const std::shared_ptr<ServiceMessage>&)>& callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  msg_callbacks_.push_back(callback);
}

void SubReactor::Mainloop() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_var_.wait(lock, [this] {return running_;});
  DLOG(INFO)<< "SubReactor [" << reactor_id_ << "] start up.";
  while (running_) {
    for (auto callback : mainloop_callbacks_) {
      callback();
    }
  }
}

#define MAXSIZE 4096
void SubReactor::OnDataRecv(const std::shared_ptr<Session>& session) {
// TODO : Read data from remote
  CHECK_NOTNULL(session.get());
  std::shared_ptr<ServiceMessage> message = std::make_shared<ServiceMessage>();
  std::shared_ptr<DataPacket> datagram = std::make_shared<DataPacket>();
  unsigned char buffer[MAXSIZE];
  do {
    int nbyte = read(session->sockfd(), buffer, sizeof(buffer));
    if (nbyte == -1 && errno == EAGAIN) {
      break;
    }
    switch (nbyte) {
      case 0: {
        DLOG(WARNING)<< "The connection [" << session->session_id()
        << "] closed by the other side.";
        ev_action_callback_(0, session);
        return;
      }
      break;
      case -1: {
        DLOG(WARNING) << "The connection occurs error, message : "
        << strerror(GetErrorCodeBySocket(session->sockfd()));
        ev_action_callback_(0, session);
        return;
      }
      break;
      default:
      break;
    }
    /* Push data to datagram */
    datagram->PushBack(buffer, nbyte);
  } while (1);
  message->set_session(session);
  message->set_datagram(datagram);
  for (auto callback : msg_callbacks_) {
    callback(message);
  }
}
#undef MAXSIZE

} /* namespace network */

#undef CHECK_STATUS

