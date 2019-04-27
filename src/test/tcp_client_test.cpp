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

class TestClientTask : public TcpClientTask {
public:
    TestClientTask(const std::string &ip, const uint16_t port) : TcpClientTask(ip, port) {
        
    };
    
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
    
    bool OnSendHelloMsg() override {
        return true;
    };
};

int main(int argc, char * argv[])
{
    //set log level
    rtc::LogMessage::LogToDebug(LS_INFO);
    
    //create a loop object
    auto main_event_loop = new EventLoop(event_base_new());
    
    //create a client task
    auto test_client_task = std::shared_ptr<TestClientTask>(new TestClientTask("127.0.0.1", 8000));
    test_client_task->BeginProcess(main_event_loop, main_event_loop);
    
    RTCHAT_LOG(LS_INFO) << "begin thread loop!";
    
    //start loop
    main_event_loop->Run();
}

