#ifndef BRKS_BUS_USERM_HANDLER_H_
#define BRKS_BUS_USERM_HANDLER_H_

#include "glo_def.h"
#include "iEventHandler.h"
#include "events_def.h"
#include "threadpool/thread.h"


class UserEventHandler :public iEventHandler {
public:
	UserEventHandler();
	virtual ~UserEventHandler();
	virtual iEvent* handle(const iEvent* ev);//专门用来处理事件，请求事件
private:
	MobileCodeRspEv* handle_mobile_code_req(MobileCodeReqEv* ev);
	LoginResEv* handle_login_req(LoginReqEv* ev);
	i32 code_gen();//产生验证码的方法
private:
	std::map<std::string, i32> m2c_;//保存手机号码对应的验证码
	pthread_mutex_t pm_;
};


#endif
