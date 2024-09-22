#pragma once
#include <Poco/Data/SessionPool.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/DataException.h>
#include <Poco/Data/RecordSet.h>
#include <spdlog/spdlog.h>

namespace mysql {

std::shared_ptr<Poco::Data::SessionPool> poco_mysql_pool;

inline void InitMysql(const std::string& conn_str, int min_sessions = 1, int max_sessions = 32, int idle_time = 60) {
  if (poco_mysql_pool) [[unlikely]] {
    spdlog::debug("mysql has been inited");
    return;
  }

  Poco::Data::MySQL::Connector::registerConnector();
  poco_mysql_pool = std::shared_ptr<Poco::Data::SessionPool>(
    new Poco::Data::SessionPool("MySQL", conn_str, min_sessions, max_sessions, idle_time),
    [](Poco::Data::SessionPool* pool) {
        spdlog::info("fini mysql");
        delete pool;
        Poco::Data::MySQL::Connector::unregisterConnector();
    });
  if (!poco_mysql_pool) {
    exit(1);
  }
  try {
    Poco::Data::Session session(poco_mysql_pool->get());
	Poco::Data::Statement statement(session);
    statement << "SHOW TABLES";
	statement.execute();
	spdlog::info("initing mysql, try exec `SHOW TABLES;`");
	// 获取结果集
    Poco::Data::RecordSet rs(statement);
	std::size_t count = rs.rowCount();
	spdlog::info("init mysql success, {} tables", count);
	return;
  } catch (Poco::Data::DataException& ex) {
    spdlog::error("MySQL Exception: {}", ex.displayText());
    throw ex;
  } catch (Poco::Exception& ex) {
	spdlog::error("Poco Exception: {}", ex.displayText());
    throw ex;
  }
}

inline Poco::Data::Statement CreateStatement() {
  Poco::Data::Session session(poco_mysql_pool->get());
  return Poco::Data::Statement(session);
}

} // namespace mysql