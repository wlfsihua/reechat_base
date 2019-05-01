//
//  UdpSocketWrapper.cpp
//  reechat
//
//  Created by raymon_wang on 2017/4/17.
//  Copyright © 2017年 wang3140@hotmail.com. All rights reserved.
//

#include "udp_socket_wrapper.hpp"

#ifdef WIN32
#include <winsock.h>
#else
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <event.h>
#include <webrtc/base/logging.h>

#include "reechat_base.h"

namespace reechat {
    #define recv_bufer_len 1536
    
    UdpSocketWrapper::UdpSocketWrapper(EventLoop* worker_loop) :
    worker_loop_(worker_loop),
    socketfd_(0),
    recv_buffer_(new char[recv_bufer_len]),
    SignalReadPacket(nullptr)
    {
        socket_read_event_ = new event;
        memset(socket_read_event_, 0, sizeof(struct event));
    }
    
    UdpSocketWrapper::~UdpSocketWrapper()
    {
        RTCHAT_LOG(LS_INFO) << Avar("UdpSocketWrapper destruction (socket_fd = %u)", socketfd_);
        worker_loop_ = nullptr;
        event_del(socket_read_event_);
        SAFE_DELETE(socket_read_event_);
        if (socketfd_ > 0) {
            EVUTIL_CLOSESOCKET(socketfd_);
        }
        SAFE_DELETEARRAY(recv_buffer_);
    }
    
    //server mode
    bool UdpSocketWrapper::BeginListen(const char* local_ip, uint16_t port)
    {
        socketfd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (socketfd_ == -1) {
            RTCHAT_LOG(LS_ERROR) << "create socket error!";
            return false;
        }
        
        evutil_make_socket_nonblocking(socketfd_);
        
        int flag = 1;
        //设置端口重用
#ifdef WIN32
		if (setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&flag), sizeof(flag)) == -1) {
			RTCHAT_LOG(LS_ERROR) << "set SO_REUSEADDR attr failed";
			return false;
	}
#else
		if (setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
			RTCHAT_LOG(LS_ERROR) << "set SO_REUSEADDR attr failed";
			return false;
	}
#endif

        struct sockaddr_in listen_addr;
        memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(port);
        listen_addr.sin_addr.s_addr = inet_addr(local_ip);
        int ret = ::bind(socketfd_, (const struct sockaddr*)&listen_addr, sizeof(struct sockaddr));
        if (ret == -1) {
            RTCHAT_LOG(LS_ERROR) << "socket bind failed";
            return false;
        }
        
        //register libevent event
        event_set(socket_read_event_, socketfd_, EV_READ|EV_PERSIST, OnReadEvent, this);
        event_base_set(worker_loop_->event_base(), socket_read_event_);
        event_add(socket_read_event_, nullptr);
        return true;
    }
    
    int UdpSocketWrapper::SendData(const void* data, size_t len, const struct sockaddr* target_addr)
    {
        if (socketfd_ == 0) {
            return false;
        }
#ifdef WIN32
        int ret = ::sendto(socketfd_, reinterpret_cast<const char*>(data), len, 0, target_addr, sizeof(struct sockaddr));
#else
		ssize_t ret = ::sendto(socketfd_, data, len, 0, target_addr, sizeof(struct sockaddr));
#endif
        return ret;
    }
    
    bool UdpSocketWrapper::CloseSocket()
    {
        event_del(socket_read_event_);
        delete this;
        return true;
    }
    
    void UdpSocketWrapper::OnReadEvent(int sockfd, short event, void* arg)
    {
        UdpSocketWrapper* socket_wrapper = static_cast<UdpSocketWrapper*>(arg);
        socket_wrapper->OnReadEvent(sockfd);
    }
    
    void UdpSocketWrapper::OnReadEvent(int sockfd)
    {
        struct sockaddr remote_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        memset(&remote_addr, 0, sizeof(remote_addr));
        int i = 0;
        while (i++ < 1000) {
#ifdef WIN32
			int num = ::recvfrom(sockfd, recv_buffer_, recv_bufer_len, 0, &remote_addr, &len);
#else
			ssize_t num = ::recvfrom(sockfd, recv_buffer_, recv_bufer_len, 0, &remote_addr, &len);
#endif
            
            if (num > 0 && SignalReadPacket) {
                SignalReadPacket(recv_buffer_, num, &remote_addr);
            }
            else {
                break;
            }
        }
    }
}


