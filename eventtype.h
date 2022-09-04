#ifndef BRKS_COMMON_EVENT_TYPE_H_
#define BRKS_COMMON_EVENT_TYPE_H_

#include "glo_def.h"
typedef struct EErrorReason_ {
	i32 code;
	const char* reason;
}EErrorReason;
/*事件ID（和messge ID是对应的）*/
//枚举类型
enum EventID {
	EEVENTID_GET_MOBILE_CODE_REQ = 0X01,
	EEVENTID_GET_MOBILE_CODE_RSP = 0X02,

	EEVENTID_LOGIN_REQ = 0X03,
	EEVENTID_LOGIN_RSP = 0X04,

	EEVENTID_RECHARGE_REQ = 0X05,
	EEVENTID_RECHARGE_RSP = 0X06,

	EEVENTID_GET_ACCOUNT_BALANCE_REQ = 0X07,
	EEVENTID_ACCOUNT_BALANCE_RSP = 0X08,

	EEVENTID_LIST_ACCOUNT_RECORDS_REQ = 0X09,
	EEVENTID_ACCOUNT_RECORDS_RSP = 0X10,

	EEVENTID_LIST_TRAVELS_REQ = 0X11,
	EEVENTID_LIST_TRAVELS_RSP = 0X12,

	EEVENTID_EXIT_RSP = 0xFE,//告诉网络接口，什么都不做，把资源释放了就行
	EEVENTID_UNKOWN = 0XFF
};
/*事件响应错误代号*/
enum EErrorCode {
	ERRC_SUCCESS = 200,
	ERRC_INVALID_MSG = 400,
	ERRC_INVALID_DATA = 404,
	ERRC_METHOD_NOT_ALLOWED = 405,
	ERRO_PROCCESS_FAILED = 406,
	ERRO_BIKE_IS_TOOK = 407,
	ERRO_BIKE_IS_RUNNING = 408,
	ERRO_BIKE_IS_DAMAGED = 409,
	ERR_NULL  = 0//added
};

const char* getReasonByErrorCode(i32 code);

#endif