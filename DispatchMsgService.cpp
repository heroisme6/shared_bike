#include "DispatchMsgService.h"
#include <algorithm>
#include "NetworkInterface.h"
#include "bike.pb.h"
#include "events_def.h"

DispatchMsgService* DispatchMsgService::DMS_ = nullptr;
std::queue<iEvent*> DispatchMsgService::response_events;//����̴߳����������¼���������Ӧ�¼�����뵽response_events
	//�У�NetworkInterface�еķ������response_events�����а���Ӧ���¼����л�����������Ӧ�����ݣ�Ȼ����Ӧ���ͻ��ˣ�
	//������Ҫ��
pthread_mutex_t DispatchMsgService::queue_mutex;//��DispatchMsgService��������Ҫ��

DispatchMsgService::DispatchMsgService():tp(nullptr) {
	//tp = NULL;
	//svr_exit_ = false;
}

DispatchMsgService::~DispatchMsgService() {

}

BOOL DispatchMsgService::open() {
	svr_exit_ = false;
	thread_mutex_create(&queue_mutex);
	tp = thread_pool_init();
	return tp ? TRUE : FALSE;
}

void DispatchMsgService::close() {
	//delete msg_queue_;
	svr_exit_ = TRUE;
	thread_pool_destroy(tp);
	thread_mutex_destroy(&queue_mutex);
	tp = NULL;
	subscribers_.clear();
}

DispatchMsgService* DispatchMsgService::getInstance() {
	if (DMS_ == nullptr) {
		DMS_ = new DispatchMsgService();
	}

	return DMS_;
}

void DispatchMsgService::svc(void* argv) {
	DispatchMsgService* dms = DispatchMsgService::getInstance();
	iEvent* ev = (iEvent*)argv;
	if (!dms->svr_exit_) {
		LOG_DEBUG("DispatchMsgService::svc...\n");
		//iEvent* rsp = dms->process(ev);
		iEvent* rsp = dms->process(ev);

		if (rsp) {
			rsp->dump(std::cout);
			rsp->set_args(ev->get_args());
			//�ŵ�������ǰ��������
		}
		else {
			rsp = new ExitRspEv();//������ֹ��Ӧ�¼�
			rsp->set_args(ev->get_args());
		}
		thread_mutex_lock(&queue_mutex);
		printf("insert element to response_events\n");
		response_events.push(rsp);
		thread_mutex_unlock(&queue_mutex);
		//ɾ���¼���������Ҫ����
		//delete ev;
	}
}

i32 DispatchMsgService::enqueue(iEvent* ev) {
	if (ev == NULL)
		return -1;

	thread_task_t* task = thread_task_alloc(0);//����һ���̳߳�����
	task->handler = DispatchMsgService::svc;//�ص�����
	task->ctx = ev;//�ص������Ĳ���	
	return thread_task_post(tp, task);//��taskͶ�ݵ��̳߳�tp��ȥ

}

//��
void DispatchMsgService::subscribe(u32 eid, iEventHandler* handler) {
	LOG_DEBUG("DispatchMsgService::subscribe eid:%u\n", eid);
	T_EventHandlersMap::iterator iter = subscribers_.find(eid);

	if (iter != subscribers_.end()) {
		T_EventHandlers::iterator hal_iter = std::find(iter->second.begin(), iter->second.end(), handler);
			if (hal_iter == iter->second.end()) {
				iter->second.push_back(handler);
			}
	}
	else {
		subscribers_[eid].push_back(handler);
	}
}
void DispatchMsgService::unsubscribe(u32 eid, iEventHandler* handler) {
	T_EventHandlersMap::iterator iter = subscribers_.find(eid);

	if (iter != subscribers_.end()) {
		T_EventHandlers::iterator hal_iter = std::find(iter->second.begin(), iter->second.end(), handler);
		if (hal_iter != iter->second.end()) {
			iter->second.erase(hal_iter);
		}
	}
}

iEvent* DispatchMsgService::process(const iEvent* ev) {
	LOG_DEBUG("DispatchMsgService::process -ev:%p\n", ev);
	if (NULL == ev) {
		return nullptr;
	}

	u32 eid = ev->get_eid();
	LOG_DEBUG("DispatchMsgService::process -eid:%p\n", eid);

	if (eid == EEVENTID_UNKOWN) {
		LOG_DEBUG("DispatchMsgService::unkown event id:%d\n", eid);
		return nullptr;
	}

	T_EventHandlersMap::iterator handlers = subscribers_.find(eid);
	if (handlers == subscribers_.end()) {
		LOG_DEBUG("DispatchMsgService::no any event handler subscribed:%d\n", eid);
		return nullptr;
	}

	iEvent* rsp = NULL;
	for (auto iter = handlers->second.begin(); iter != handlers->second.end(); iter++) {
		iEventHandler* handler = *iter;
		LOG_DEBUG("DispatchMsgService::get handler:%s\n", handler->get_name().c_str());

		rsp = handler->handle(ev);//�Ƽ�ʹ��vector��list���ض��rsp
	}

	return rsp;
}

iEvent* DispatchMsgService::parseEvent(const char* message, u32 len, u32 eid) {
	if (!message) {
		LOG_ERROR("DispatchMsgService::parseEvent = messageis null[eid:%d],\n", eid);
		return nullptr;
	}

	if (eid == EEVENTID_GET_MOBILE_CODE_REQ) {
		tutorial::mobile_request mr;
		if (mr.ParseFromArray(message, len)) {
			MobileCodeReqEv* ev = new MobileCodeReqEv(mr.mobile());
			return ev;
		}
	}
	else if (eid == EEVENTID_LOGIN_REQ) {//���������¼���EEVENTID_LOGIN_REQ���Ǿͽ�����Ӧ�Ĵ���
		tutorial::login_request lr;
		if (lr.ParseFromArray(message, len)) {
			LoginReqEv* ev = new LoginReqEv(lr.mobile(), lr.icode());
			return ev;
		}
		//δ�����
	}
	return nullptr;
}

void DispatchMsgService::handleAllResponseEvent(NetworkInterface* interface) {
	bool done = false;
	while (!done) {
		iEvent* ev = nullptr;
		//��Ҫʹ��ͬ�����⣬��Ϊ�ж���̲߳��ϵĽ��¼�Ͷ�ݽ����У�63�У��������̲��϶�ȡ�����е��¼���������Ҫ
		//�������
		thread_mutex_lock(&queue_mutex);
		if (!response_events.empty()) {
			ev = response_events.front();
			response_events.pop();
		}
		else {
			done = true;
		}
		thread_mutex_unlock(&queue_mutex);
		
		if (!done) {
			if (ev->get_eid() == EEVENTID_GET_MOBILE_CODE_RSP) {
				//MobileCodeRspEv* mcre = static_cast<MobileCodeRspEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent-id:EEVENTID_GET_MOBILE_CODE_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();
				cs->response = ev;

				//ϵ�л���������
				cs->message_len = ev->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				//��װͷ��
				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EEVENTID_GET_MOBILE_CODE_RSP;
				*(i32*)(cs->write_buf + 6) = cs->message_len;

				ev->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);
				interface->send_response_message(cs);

			}else if (ev->get_eid() == EEVENTID_EXIT_RSP) {
				ConnectSession* cs = (ConnectSession*)ev->get_args();
				cs->response = ev;
				interface->send_response_message(cs);
			}
			else if (ev->get_eid() == EEVENTID_LOGIN_RSP) {
				//LoginResEv* lgre = static_cast<LoginResEv*>(ev);
				LOG_DEBUG("DispatchMsgService::handleAllResponseEvent - id:EEVENTID_LOGIN_RSP\n");

				ConnectSession* cs = (ConnectSession*)ev->get_args();
				cs->response = ev;

				//ϵ�л���������
				cs->message_len = ev->ByteSize();
				cs->write_buf = new char[cs->message_len + MESSAGE_HEADER_LEN];

				//��װͷ��
				memcpy(cs->write_buf, MESSAGE_HEADER_ID, 4);
				*(u16*)(cs->write_buf + 4) = EEVENTID_LOGIN_RSP;
				*(i32*)(cs->write_buf + 6) = cs->message_len;

				ev->SerializeToArray(cs->write_buf + MESSAGE_HEADER_LEN, cs->message_len);

				interface->send_response_message(cs);
			}
		}
	}
}