#ifndef  BRKS_BUS_MAIN_H_
#define BRKS_BUS_MAIN_H_
#include "user_event_handler.h"
#include "sqlconnection.h"
class BussinessProcessor {
public:
	BussinessProcessor(std::shared_ptr<MysqlConnection> conn);
	bool init();//如果表没创建，用这个创建
	virtual ~BussinessProcessor();
private:
	std::shared_ptr<MysqlConnection> mysqlconn_;
	std::shared_ptr<UserEventHandler> ueh_;//账户管理系统
};
#endif // ! BRKS_BUS_MAIN_H_

