/*DispatchMsgService - 负责分发消息服务模块，其实就是把外部收到的消息，转化成内部事件，
也就是data->msg->event的解码过程，然后再把事件投递至线程池的消息队列，
由线程池调用其process 方法对事件进行处理，最终调用每个	
event的handler方法来处理event，此时每个event handler需要subscribe该event后才会被调用到.*/

#ifndef BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_
#define BRK_SERVICE_DISPATCH_EVENT_SERVICE_H_
#include <map>
#include <vector>
#include "ievent.h"
#include "eventtype.h"
#include "iEventHandler.h"
#include "threadpool/thread_pool.h"
#include <queue>
#include "NetworkInterface.h"

class DispatchMsgService {
protected:
	DispatchMsgService();
public:
	//DispatchMsgService();
	virtual ~DispatchMsgService();

	virtual BOOL open();
	virtual void close();

	virtual void subscribe(u32 eid, iEventHandler* handler);
	virtual void unsubscribe(u32 eid, iEventHandler* handler);

	/*把事件投递到线程池中进行处理*/
	virtual i32 enqueue(iEvent* ev);
	//线程池回调函数
	static void svc(void* argv);
	//对具体事件进行分发处理
	virtual iEvent* process(const iEvent* ev);
	static DispatchMsgService* getInstance();//单例模式
	iEvent* parseEvent(const char* message, u32 len, u32 eid);

	void handleAllResponseEvent(NetworkInterface* interface);
protected:
	thread_pool_t* tp;

	static DispatchMsgService* DMS_;

	typedef std::vector<iEventHandler*> T_EventHandlers;
	typedef std::map<u32, T_EventHandlers> T_EventHandlersMap;
	T_EventHandlersMap subscribers_;

	bool svr_exit_;//调用open服务开始，调用cloase服务结束

	static std::queue<iEvent*> response_events;//多个线程处理完请求事件，产生响应事件会插入到response_events
	//中，NetworkInterface中的方法会从response_events队列中把响应的事件序列化成网络上响应的数据，然后响应给客户端，
	//所以需要锁
	static pthread_mutex_t queue_mutex;

};


#endif