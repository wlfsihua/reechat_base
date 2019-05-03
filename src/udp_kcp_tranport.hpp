//
//  udp_kcp_tranport.hpp
//  reechat_base
//
//  Created by Raymon on 28/04/2019.
//  Copyright © 2019 ztgame. All rights reserved.
//

#ifndef udp_kcp_tranport_hpp
#define udp_kcp_tranport_hpp

#include <stdio.h>
#include <memory>
#include <sys/socket.h>

struct IKCPCB;

namespace reechat {
    class UdpSocketWrapper;
    class EventLoop;
    class UdpKcpTransport : public std::enable_shared_from_this<UdpKcpTransport> {
    public:
        UdpKcpTransport(EventLoop* worker_loop);
        bool BeginListen(const char* listen_ip, uint16_t listen_port, uint32_t transport_id);
        void SetSendDestination(const char* dest_ip, uint16_t dest_port);
        bool SendData(const char* data, size_t len);
        virtual void ScheduleEvent(int64_t current_time);
        
        void OndataReceived(const char* data, size_t len, const struct sockaddr* src_dest);
        
    private:
        UdpSocketWrapper*   udp_socket_wrapper_;
        IKCPCB*             ikcp_context_;
        struct sockaddr     dest_addr_;
    };
}

#endif /* udp_kcp_tranport_hpp */
