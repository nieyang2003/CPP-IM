#include "smtp.h"
#include <Poco/Net/MailMessage.h>
#include <Poco/Net/NetException.h>
#include <iostream>
#include <spdlog/spdlog.h>

namespace {

DEFINE_uint32(max_smtp_seesions, 4, "smtp session最大数量");
DEFINE_string(smtp_server_host, "smtp.qq.com", "smtp服务域名");
DEFINE_uint32(smtp_server_port, 587, "smtp服务端口");
DEFINE_string(smtp_user, "3536448549@qq.com", "smtp用户");
DEFINE_string(smtp_password, "iwxwlkukubvhchjg", "smtp密码");

} // namespace

bool varify::SmtpPoll::Send(const std::string &recipient, const std::string &subject, const std::string &content) {
  std::unique_ptr<Poco::Net::SMTPClientSession> session;

  Poco::Net::MailMessage message;
  message.setSender(sender_);
  message.addRecipient(Poco::Net::MailRecipient(Poco::Net::MailRecipient::PRIMARY_RECIPIENT, recipient));
  message.setSubject(subject);
  message.setContent("您的验证码为： " + content + "，请于3分钟内完成注册。");

  session = Acquire();
  for (int i = 0; i < 3; ++i) {
    try {
  	  session->sendMessage(message);
  	  Release(std::move(session));
  	  return true;
    } catch (Poco::Net::SMTPException& e) {
  	  spdlog::error("Poco::Exception: {}，重试一次", e.displayText());
	  session->close();
  	  session = CreateConnection();
    } catch (const Poco::Exception& e) {
  	  spdlog::error("Poco::Exception: {}", e.displayText());
    }
  }
  return false;
}

varify::SmtpPoll::SmtpPoll()
  : sender_(FLAGS_smtp_user) {
  for (size_t i = 0; i < FLAGS_max_smtp_seesions; ++i) {
    pool_.push(std::move(CreateConnection()));
  }
}

std::unique_ptr<Poco::Net::SMTPClientSession> varify::SmtpPoll::CreateConnection() {
  try {
    auto session = std::make_unique<Poco::Net::SMTPClientSession>(FLAGS_smtp_server_host, FLAGS_smtp_server_port);
    if (!session) {
	  spdlog::error("创建session失败");
      exit(1);
    }
    session->login(Poco::Net::SMTPClientSession::AUTH_LOGIN, FLAGS_smtp_user, FLAGS_smtp_password);
	return session;
  } catch (Poco::Net::SMTPException& e) {
    // 处理SMTP协议相关的错误，例如身份验证失败
	spdlog::error("SMTPException: {}", e.displayText());
  } catch (Poco::Net::NetException& e) {
    // 处理网络相关的错误，例如无法连接到服务器
	spdlog::error("NetException: {}", e.displayText());
  } catch (Poco::Exception& e) {
    // 处理其他错误
	spdlog::error("Exception: {}", e.displayText());
  }
  exit(1);
}
