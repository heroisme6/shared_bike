#include "BusProcessor.h"
#include "SqlTables.h"

BussinessProcessor::BussinessProcessor(std::shared_ptr<MysqlConnection> conn):mysqlconn_(conn), ueh_(new UserEventHandler())
{

}

bool BussinessProcessor::init() {
	SqlTables tables(mysqlconn_);
	tables.CreateUserInfo();
	tables.CreateBikeTable();
	return true;
}

BussinessProcessor::~BussinessProcessor()
{
	ueh_.reset();
}