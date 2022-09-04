#include <iostream>
#include <unistd.h>
#include "bike.pb.h"
#include "ievent.h"
#include "eventtype.h"
#include "events_def.h"
#include "user_event_handler.h"
#include "DispatchMsgService.h"
#include "NetworkInterface.h"

#include "iniconfig.h"
#include "Logger.h"
#include "sqlconnection.h"
#include "SqlTables.h"
#include "BusProcessor.h"
#include "memory"
using namespace std;

int main(int argc, char** argv) {
	if (argc != 3) {
		printf("Please input shbk <config file path> <log file config!\n>");
		return -1;
	}

	//初始化日志
	if (!Logger::instance()->init(std::string(argv[2]))) {
		fprintf(stderr, "init log module failed\n");
		return -2;
	}

	Iniconfig *config = Iniconfig::getInstance();
	if (!config->loadfile(std::string(argv[1]))) {
		//printf("load %s faild.\n", argv[1]);
		LOG_ERROR("load %s failed.", argv[1]);
		Logger::instance()->GetHandle()->error("load %s failed.", argv[1]);//和上面一行，作用相同
		return -3;
	}
	st_env_config conf_args = config->getconfig();
	LOG_INFO("[database]: %s, %d, %s, %s, %s, %d\n", conf_args.db_ip.c_str(), conf_args.db_port, conf_args.db_user.c_str(), conf_args.db_pwd.c_str(), \
		conf_args.db_name.c_str(), conf_args.svr_port);
	//tutorial::mobile_request msg;
	//msg.set_mobile("18131139503");
	//iEvent *ie = new iEvent(EEVENTID_GET_MOBILE_CODE_REQ, 2);  

	//MobileCodeReqEv me("18131139506");
	//me.dump(cout);
	//cout << msg.mobile() << endl;

	//MobileCodeRspEv mcre1(200, 66666);
	//mcre1.dump(cout);

	//MobileCodeRspEv mcre2(ERRC_BIKE_IS_TOOK, 66666);
	//mcre2.dump(cout);

	//MobileCodeRspEv mcre3(500, 66666);
	//mcre3.dump(cout);
	//MysqlConnection mysqlConn;
	std::shared_ptr<MysqlConnection> mysqlconn(new MysqlConnection); 
	if (!mysqlconn->Init(conf_args.db_ip.c_str(), conf_args.db_port, conf_args.db_user.c_str(),
		conf_args.db_pwd.c_str(), conf_args.db_name.c_str())) {
		LOG_ERROR("Database init failed.\n");
		return -4;
	}


	BussinessProcessor busPro(mysqlconn);//在构造时会生成用户事件处理的对象，用户事件处理对象在构造时会订阅事件
	busPro.init();//如果表不存在就创建表，否则就不创建表

	//UserEventHandler uehl;
	//uehl.handle(&me);
	DispatchMsgService* DMS = DispatchMsgService::getInstance();
	DMS->open();//创建线程池

	NetworkInterface* NTIF = new NetworkInterface();
	NTIF->start(8888);
	while (1 == 1) {
		NTIF->network_event_dispatch();
		sleep(1);
		LOG_DEBUG("network_event_dispatch...\n");
	}
	/*MobileCodeReqEv* pmcre = new MobileCodeReqEv("18131139503");
	DMS->enqueue(pmcre);//把事件投递到线程池中去*/
	sleep(5);
	DMS->close();
	sleep(5);

	return 0;
}