/*
 * SubReactor.h
 *
 *  Created on: 2017年12月6日
 *      Author: xueda
 */

#ifndef SRC_NETWORK_SUBREACTOR_H_
#define SRC_NETWORK_SUBREACTOR_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace network {

class Session;
class SessionManager;
class ServiceContext;

class SubReactor {
 public:
  SubReactor(SessionManager* session_manager);
  virtual ~SubReactor();

  const std::string& reactor_id() const;

  void Start();
  void Stop();
  void AddMainloopCallback(const std::function<void()>& callback);
  void AddPushMessageCallback(
      const std::function<void(const std::shared_ptr<ServiceContext>&)>& callback);
  void AddEventActionCallback(
      const std::function<void(int, const std::shared_ptr<Session>&)>& callback);
  void OnDataRecv(const std::shared_ptr<Session> &session);

 private:
  void Mainloop();

  bool running_;
  std::thread* reactor_;
  std::mutex mutex_;
  std::string reactor_id_;
  std::condition_variable cond_var_;
  SessionManager* session_manager_;
  std::vector<std::function<void(int, const std::shared_ptr<Session>&)>> ev_action_callbacks_;
  std::vector<std::function<void()>> mainloop_callbacks_;
  std::vector<std::function<void(const std::shared_ptr<ServiceContext>&)>> push_msg_callbacks_;
};

} /* namespace network */

#endif /* SRC_NETWORK_SUBREACTOR_H_ */
