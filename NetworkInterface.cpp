#include "NetworkInterface.h"
#include <stdlib.h>
#include <string.h>
#include "DispatchMsgService.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//切记：ConnectSession必须是C语言类型的成员变量
static ConnectSession* session_init(int fd, struct bufferevent* bev) {
	//static作用：只是在这个文件可见，在别的文件里不能调用
	ConnectSession* temp = nullptr;
	temp = new ConnectSession();
	if (!temp) {
		fprintf(stderr, "malloc failed. reason:%m\n");
		return nullptr;
	}

	memset(temp, '\0', sizeof(ConnectSession));
	temp->bev = bev;
	temp->fd = fd;
}

void session_free(ConnectSession* cs) {
	if (cs) {
		if (cs->read_buf) {
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}

		if (cs->write_buf) {
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}
		delete cs;
	}
}

void NetworkInterface::session_reset(ConnectSession* cs) {
	if (cs) {
		if (cs->read_buf) {
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
		}

		if (cs->write_buf) {
			delete[] cs->write_buf;
			cs->write_buf = nullptr;
		}

		cs->session_stat = SESSION_STATUS::SS_REQUEST;
		cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER;

		cs->message_len = 0;
		cs->read_message_len = 0;
		cs->read_header_len = 0;
		
	}
}
NetworkInterface::NetworkInterface() {
	base_ = nullptr;
	listener_ = nullptr;
}

NetworkInterface::~NetworkInterface() {
	close();
}

bool NetworkInterface::start(int port) {
	printf("enter start\n");
	struct sockaddr_in sin;
	memset(&sin, 0, sizeof(struct sockaddr_in));//0.0.0.0
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	base_ = event_base_new();
	listener_
		= evconnlistener_new_bind(base_, NetworkInterface::listener_cb, base_,
			LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
			512, (struct sockaddr*)&sin,
			sizeof(struct sockaddr_in));//listener_cb是回调函数
	printf("over start\n");
}

void NetworkInterface::close() {
	if (base_) {
		event_base_free(base_);
		base_ = nullptr;
	}

	if (listener_) {
		evconnlistener_free(listener_);
		listener_ = nullptr;
	}
}

void NetworkInterface::listener_cb(struct evconnlistener* listener, evutil_socket_t fd,
	struct sockaddr* sock, int socklen, void* arg) {
	struct event_base* base = (struct event_base*)arg;
	LOG_DEBUG("accept a client %d\n", fd);

	//为这个客户端分配一个bufferevent
	struct bufferevent* bev = bufferevent_socket_new(base, fd,
		BEV_OPT_CLOSE_ON_FREE);// BEV_OPT_CLOSE_ON_FREE：当关闭bufferevent时，会关闭socket和free

	ConnectSession* cs = session_init(fd, bev);
	cs->session_stat = SESSION_STATUS::SS_REQUEST;
	cs->req_stat = MESSAGE_STATUS::MS_READ_HEADER;
	strcpy(cs->remote_ip, inet_ntoa(((sockaddr_in*)sock)->sin_addr));//字节序将地址存放到remote_ip中
	LOG_DEBUG("remote ip:%s\n", cs->remote_ip);

	bufferevent_setcb(bev, handle_request, handle_response, handle_error, cs);//handle_request:处理请求的
	//handle_response数据发送成功以后的回调，handle_error：出错以后的回调
	bufferevent_enable(bev, EV_READ | EV_PERSIST);//允许读且允许持续的读取
	bufferevent_settimeout(bev, 10, 10);//超时时应设置配置文件
	printf("bufferevent_enable\n");
}

void NetworkInterface::handle_request(struct bufferevent* bev, void* arg) {
	LOG_WARN("enter handle_request");
	ConnectSession* cs = (ConnectSession*)arg;

	if (cs->session_stat != SESSION_STATUS::SS_REQUEST) {
		LOG_WARN("NetworkInterface::handle_request - wrong session state[%d].", cs->session_stat);
		return;
	}

	if (cs->req_stat == MESSAGE_STATUS::MS_READ_HEADER) {
		i32 len = bufferevent_read(bev, cs->header + cs->read_header_len, MESSAGE_HEADER_LEN - cs->read_header_len);
		cs->read_header_len += len;

		cs->header[cs->read_header_len] = '\0';
		LOG_DEBUG("recv from client<<<<%s\n", cs->header);

		if (cs->read_header_len == MESSAGE_HEADER_LEN) {
			if (strncmp(cs->header, MESSAGE_HEADER_ID, strlen(MESSAGE_HEADER_ID)) == 0) {
				cs->eid = *((u16*)(cs->header + 4));
				cs->message_len = *((i32*)(cs->header + 6));

				LOG_DEBUG("NetworkInterface::handle_request - read %d bytes in header, messgage len%d\n", cs->read_header_len, cs->message_len);
				if (cs->message_len < 1 || cs->message_len > MAX_MESSAGE_LEN) {
					LOG_ERROR("NetworkInterface::handle_request - wrong message,len:%u\n", cs->message_len);
					bufferevent_free(bev);
					session_free(cs);
					return;
				}

				cs->read_buf = new char[cs->message_len];
				cs->req_stat = MESSAGE_STATUS::MS_READ_MESSAGE;
				cs->read_message_len = 0;
			}
			else {
				LOG_ERROR("NetworkInterface::handle_request - Invalid request from %s\n", cs->remote_ip);
				//直接关闭请求，不给予任何请求，防止客户端恶意试探
				bufferevent_free(bev);
				session_free(cs);
				return;
			}
		}
	}

	if (cs->req_stat == MESSAGE_STATUS::MS_READ_MESSAGE && evbuffer_get_length(bufferevent_get_input(bev)) > 0) {
		i32 len = bufferevent_read(bev, cs->read_buf + cs->read_message_len, cs->message_len - cs->read_message_len);
		cs->read_message_len += len;
		LOG_DEBUG("NetworkInterface::handle_request - bufferevent_read:%d bytes, message len:%d, read len:%d\n", len, cs->message_len, cs->read_message_len);
		if (cs->message_len == cs->read_message_len) {
			cs->session_stat = SESSION_STATUS::SS_RESPONSE;
			iEvent* ev = DispatchMsgService::getInstance()->parseEvent(cs->read_buf, cs->read_message_len, cs->eid);
			delete[] cs->read_buf;
			cs->read_buf = nullptr;
			cs->read_message_len = 0;

			if (ev) {
				ev->set_args(cs);
				printf("enqueue(ev)\n");
				DispatchMsgService::getInstance()->enqueue(ev);//投递到线程池中去处理
			}
			else {
				LOG_ERROR("NetworkInterface::handle_request - ev is null, remote ip: %s, eid:%d\n", cs->remote_ip, cs->eid);
				//直接关闭请求，不给予任何请求，防止客户端恶意试探
				bufferevent_free(bev);
				session_free(cs);
				return;
			}

		}
	}
}

void NetworkInterface::handle_response(struct bufferevent* bev, void* arg) {
	LOG_DEBUG("NetworkInterface::handle_response...\n");
}

//超时、连接关闭、读写出错等异常情况指定的回调函数
void NetworkInterface::handle_error(struct bufferevent* bev, short event, void* arg) {
	ConnectSession* cs = (ConnectSession*)arg;
	LOG_DEBUG("NetworkInterface::handle_error...\n");
	if (event & BEV_EVENT_EOF) {
		LOG_WARN("NetworkInterface::connection closed\n");
	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_READING)) {
		LOG_WARN("NetworkInterface::reading timeout\n");
	}
	else if ((event & BEV_EVENT_TIMEOUT) && (event & BEV_EVENT_WRITING)) {
		LOG_WARN("NetworkInterface::writing timeout\n");
	}
	else if (event & BEV_EVENT_ERROR) {
		LOG_ERROR("NetworkInterface::some other error.....\n");
	}

	bufferevent_free(bev);
	session_free(cs);
}
void NetworkInterface::network_event_dispatch() {
	event_base_loop(base_, EVLOOP_NONBLOCK);//EVLOOP_NOBLOCK：非阻塞
	//处理响应事件，回复响应消息-未完待续
	DispatchMsgService::getInstance()->handleAllResponseEvent(this);
}

void NetworkInterface::send_response_message(ConnectSession* cs) {
	if (cs->response == nullptr) {
		bufferevent_free(cs->bev);
		if (cs->request) {
			delete cs->request;
		}
		session_free(cs);
	}
	else {
		bufferevent_write(cs->bev, cs->write_buf, cs->message_len + MESSAGE_HEADER_LEN);
		session_reset(cs);
	}
}