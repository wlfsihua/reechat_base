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
    
    bool UdpKcpTransport::BeginListen(const char *listen_ip, uint16_t listen_port, uint32_t transport_id)
    {
        udp_socket_wrapper_->BeginListen(listen_ip, listen_port);
        udp_socket_wrapper_->SignalReadPacket = std::bind(&UdpKcpTransport::OndataReceived, this, _1, _2, _3);
        
        auto share_this_helper_ = new CShareThisHelper<UdpKcpTransport>(shared_from_this());
        ikcp_context_ = ikcp_create(transport_id, share_this_helper_);
        ikcp_nodelay(ikcp_context_, 0, 40, 0, 0);
        
        ikcp_setoutput(ikcp_context_, [](const char* data, int len, ikcpcb*, void* ptr)->int{
            //send data to network layer
            auto transport = static_cast<CShareThisHelper<UdpKcpTransport>*>(ptr);
            
            return transport->socket_wrapper->udp_socket_wrapper_->SendData(data, len, &transport->socket_wrapper->dest_addr_);
        });
        return true;
    }
    
    void UdpKcpTransport::SetSendDestination(const char* dest_ip, uint16_t dest_port)
    {
        (*(struct sockaddr_in*)&dest_addr_).sin_family = AF_INET;
        (*(struct sockaddr_in*)&dest_addr_).sin_port = htons(dest_port);
        (*(struct sockaddr_in*)&dest_addr_).sin_addr.s_addr = inet_addr(dest_ip);
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
        ikcp_update(ikcp_context_, current_time);
        
        char buffer[1500] = {0};
        while (true) {
            int len = ikcp_recv(ikcp_context_, buffer, 1500);
            if (len < 0) {
                break;
            }

//            RTCHAT_LOG(LS_INFO) << "recv data len:" << len;
            //Post message to appication layer
            //Todo...
        }
    }
    
    void UdpKcpTransport::OndataReceived(const char* data, size_t len, const struct sockaddr* src_dest)
    {
        ikcp_input(ikcp_context_, data, len);
    }
}
