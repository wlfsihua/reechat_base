//
//  udp_kcp_tranport.cpp
//  reechat_base
//
//  Created by Raymon on 28/04/2019.
//  Copyright © 2019 ztgame. All rights reserved.
//

#include "udp_kcp_tranport.hpp"

#include "third_party/kcp/ikcp.h"

#include "rtc_log_wrapper.hpp"
#include "udp_socket_wrapper.hpp"
#include "macros.h"

namespace reechat {
    UdpKcpTransport::UdpKcpTransport(EventLoop* worker_loop) :
    udp_socket_wrapper_(new UdpSocketWrapper(worker_loop)),
    ikcp_context_(nullptr)
    {
        
    }
    
    bool UdpKcpTransport::Init()
    {
        auto share_this_helper_ = new CShareThisHelper<UdpKcpTransport>(shared_from_this());
        ikcp_context_ = ikcp_create(11223344, share_this_helper_);
        ikcp_setoutput(ikcp_context_, [](const char* data, int len, ikcpcb*, void* ptr)->int{
            //send data to network layer
            auto transport = static_cast<CShareThisHelper<UdpKcpTransport>*>(ptr);
            return transport->socket_wrapper->SendData(data, len);
        });
        return true;
    }
    
    bool UdpKcpTransport::SendData(const char* data, size_t len)
    {
        if (ikcp_send(ikcp_context_, data, len) < 0) {
            RTCHAT_LOG(LS_INFO) << "ikcp_send error";
            return false;
        }
        return true;
    }
    
    void UdpKcpTransport::ScheduleEvent(int64_t current_time)
    {
        if (!ikcp_context_) {
            return;
        }
        ikcp_update(ikcp_context_, 10);
        
        char buffer[2048] = {0};
        while (true) {
            int len = ikcp_recv(ikcp_context_, buffer, 2048);
            if (len < 0) {
                break;
            }
            //Post message to appication layer
            //Todo...
        }
    }
    
    void UdpKcpTransport::OndataReceived(const char* data, size_t len)
    {
        ikcp_input(ikcp_context_, data, len);
    }
}
