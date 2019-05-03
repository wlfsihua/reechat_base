//
//  udp_kcp_test.cpp
//  reechat_base
//
//  Created by Raymon on 29/04/2019.
//  Copyright © 2019 ztgame. All rights reserved.
//

#include "../reechat_base.h"

#include <../third_party/libevent2/include/event.h>
#include <webrtc/rtc_base/timeutils.h>

using namespace reechat;

TimerEventWatcher*  s_send_schedule_timer = nullptr;
TimerEventWatcher*  s_recv_schedule_timer = nullptr;

void SendTransportTest()
{
    //create a loop object
    auto send_network_loop = new EventLoop(event_base_new());
    
    auto kcp_send_transport = std::shared_ptr<UdpKcpTransport>(new UdpKcpTransport(send_network_loop));
    kcp_send_transport->BeginListen("127.0.0.1", 10000, 11223344);
    kcp_send_transport->SetSendDestination("127.0.0.1", 20000);
    
    s_send_schedule_timer = new TimerEventWatcher(send_network_loop, [kcp_send_transport](){
        kcp_send_transport->ScheduleEvent(rtc::TimeMillis());
        s_send_schedule_timer->AsyncWait();
    }, 100);
    s_send_schedule_timer->Init();
    s_send_schedule_timer->AsyncWait();
    
    new std::thread([send_network_loop](){
        send_network_loop->Run();
    });
    
    char buffer[1024];
    while (true) {
        for (int i = 0; i < 10; i++) {
            kcp_send_transport->SendData(buffer, 1024);
        }
        usleep(1000 * 10);
    }
}

void RecvTransportTest()
{
    //create a loop object
    auto recv_network_loop = new EventLoop(event_base_new());
    
    auto kcp_recv_transport = std::shared_ptr<UdpKcpTransport>(new UdpKcpTransport(recv_network_loop));
    kcp_recv_transport->BeginListen("127.0.0.1", 20000, 11223344);
    kcp_recv_transport->SetSendDestination("127.0.0.1", 10000);
    
    s_recv_schedule_timer = new TimerEventWatcher(recv_network_loop, [kcp_recv_transport](){
        kcp_recv_transport->ScheduleEvent(rtc::TimeMillis());
        s_recv_schedule_timer->AsyncWait();
    }, 100);
    s_recv_schedule_timer->Init();
    s_recv_schedule_timer->AsyncWait();
    
    new std::thread([recv_network_loop](){
        recv_network_loop->Run();
    });
}

int main(int argc, char * argv[])
{
    //set log level
    rtc::LogMessage::LogToDebug(LS_INFO);
    
    RecvTransportTest();
    SendTransportTest();
}
