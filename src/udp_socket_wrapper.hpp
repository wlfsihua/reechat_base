//
//  UdpSocketWrapper.hpp
//  reechat
//
//  Created by raymon_wang on 2017/4/17.
//  Copyright © 2017年 wang3140@hotmail.com. All rights reserved.
//

#ifndef UdpSocketWrapper_hpp
#define UdpSocketWrapper_hpp

#include "event_loop.hpp"

#include <iostream>
#include <functional>
#include <string.h>
#include <sys/types.h>

#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#endif

#include "socket_buffer.hpp"

using namespace std::placeholders;

struct event;

namespace reechat {
    class UdpSocketWrapper {
        typedef std::function<void (const char*, size_t, const struct sockaddr*)> t_SignalReadPacket;
    public:
        UdpSocketWrapper(EventLoop* worker_loop);
        ~UdpSocketWrapper();
        
        //server mode
        bool BeginListen(const char* local_ip, uint16_t port);
        
        int SendData(const void* data, size_t len, const struct sockaddr* target_addr);
        
        bool CloseSocket();
        
    public:
        t_SignalReadPacket SignalReadPacket;
        
    private:
        static void OnReadEvent(int sockfd, short event, void* arg);
        void OnReadEvent(int sockfd);
        
    private:
        EventLoop*              worker_loop_;
        struct event*           socket_read_event_;
        int                     socketfd_;
        char*                   recv_buffer_;
    };
}



#endif /* UdpSocketWrapper_hpp */
