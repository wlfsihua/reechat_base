//
//  tcp_server_test.cpp
//  reechat_base
//
//  Created by Raymon on 24/04/2019.
//  Copyright © 2019 ztgame. All rights reserved.
//

#include "../reechat_base.h"

#include <../third_party/libevent2/include/event.h>

using namespace reechat;

class TestServerTask : public TcpServerTask {
public:
    
protected:
    bool OnRecvVerifyMsg(const char* data, size_t size) override {return true;};
    void MsgParse(const char* data, size_t size) override {
        //Write msg process logic here
        //Todo...
    };
    void AddToManager() override {
        //can add this server task to a manager here, should not call by user
    };
    void RemoveFromManager() override {
        //can remove this server task to a manager here, should not call by user
    };
};

int main(int argc, char * argv[])
{
    //set log level
    rtc::LogMessage::LogToDebug(LS_INFO);
    
    //create a loop object
    auto main_event_loop = new EventLoop(event_base_new());
    //create a listener socket
    auto listen_socket = std::shared_ptr<TcpSocketWrapper>(new TcpSocketWrapper(main_event_loop, main_event_loop));
    listen_socket->BeginListen("0.0.0.0", 8000);
    //define process function of the listener socket
    listen_socket->SignalNewConnection = [main_event_loop](std::shared_ptr<TcpSocketWrapper> listen_socket){
        //accept new connection
        auto new_ev_socket = listen_socket->AcceptConnection(main_event_loop);
        auto test_server_task = std::shared_ptr<TestServerTask>(new TestServerTask());
        test_server_task->BeginProcess(new_ev_socket);
    };
    
    RTCHAT_LOG(LS_INFO) << "begin thread loop!";
    
    //start loop
    main_event_loop->Run();
}

