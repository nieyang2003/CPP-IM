#pragma once
#include <gflags/gflags.h>
#include <Poco/Net/SMTPClientSession.h>
#include <Poco/Timer.h>
#include <string>
#include "../include/singleton.h"
#include "../include/connection_pool.h"

namespace varify {

class SmtpPoll : protected ConnectionPool<Poco::Net::SMTPClientSession>, public Singleton<SmtpPoll> {
  friend class Singleton<SmtpPoll>;
 public:
  bool Send(const std::string& recipient, const std::string& subject, const std::string& content);

 protected:
  SmtpPoll();
  std::unique_ptr<Poco::Net::SMTPClientSession> CreateConnection() override;
//   void OnTimerReconnect(Poco::Timer& timer);

 private:
  Poco::Timer timer_;
  std::string sender_;
};

}