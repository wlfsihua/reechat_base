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

TimerEventWatcher*  s_schedule_timer;

int main(int argc, char * argv[])
{
    //set log level
    rtc::LogMessage::LogToDebug(LS_INFO);
    
    //create a loop object
    auto main_event_loop = new EventLoop(event_base_new());
    auto network_loop = new EventLoop(event_base_new());
    
    auto kcp_transport = std::shared_ptr<UdpKcpTransport>(new UdpKcpTransport(network_loop));
    kcp_transport->Init();
    
    s_schedule_timer = new TimerEventWatcher(network_loop, [kcp_transport](){
        kcp_transport->ScheduleEvent(rtc::TimeMillis());
        s_schedule_timer->AsyncWait();
        
    }, 10);
    s_schedule_timer->Init();
    s_schedule_timer->AsyncWait();
    
    new std::thread([network_loop](){
        network_loop->Run();
    });
    
    char buffer[1024];
    while (true) {
        kcp_transport->SendData(buffer, 1024);
        usleep(1000 * 20);
    }
}
