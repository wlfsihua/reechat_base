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

struct IKCPCB;

namespace reechat {
    class UdpSocketWrapper;
    class EventLoop;
    class UdpKcpTransport : public std::enable_shared_from_this<UdpKcpTransport> {
    public:
        UdpKcpTransport(EventLoop* worker_loop);
        bool Init();
        bool SendData(const char* data, size_t len);
        virtual void ScheduleEvent(int64_t current_time);
        
        void OndataReceived(const char* data, size_t len);
        
    private:
        UdpSocketWrapper*   udp_socket_wrapper_;
        IKCPCB*             ikcp_context_;
    };
}

#endif /* udp_kcp_tranport_hpp */
