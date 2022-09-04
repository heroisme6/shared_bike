#ifndef BRKS_SVR_USER_SERVICE_H_
#define BRKS_SVR_USER_SERVICE_H_
#include "glo_def.h"
#include "sqlconnection.h"
#include "events_def.h"

#include <memory>

class UserService {
public:
	UserService(std::shared_ptr<MysqlConnection> sql_conn);
	bool exist(const std::string& mobile);//判断手机存在否，不存在则调用insert
	bool insert(const std::string& mobile);
private:
	std::shared_ptr<MysqlConnection> sql_conn_;
};

#endif
