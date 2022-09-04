#ifndef  BRKS_BUS_MAIN_H_
#define BRKS_BUS_MAIN_H_
#include "user_event_handler.h"
#include "sqlconnection.h"
class BussinessProcessor {
public:
	BussinessProcessor(std::shared_ptr<MysqlConnection> conn);
	bool init();//�����û���������������
	virtual ~BussinessProcessor();
private:
	std::shared_ptr<MysqlConnection> mysqlconn_;
	std::shared_ptr<UserEventHandler> ueh_;//�˻�����ϵͳ
};
#endif // ! BRKS_BUS_MAIN_H_

