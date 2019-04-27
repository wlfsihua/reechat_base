//
//  tcp_client_task.hpp
//  SignalServer
//
//  Created by raymon_wang on 2018/3/9.
//  Copyright © 2018年 reechat studio. All rights reserved.
//

#ifndef TcpClientTask_hpp
#define TcpClientTask_hpp

#include "tcp_socket_wrapper.hpp"

#include <iostream>
#include <memory>

namespace reechat {
    class TcpSocketWrapper;
    class TimerEventWatcher;
    class EventLoop;
    
    class TcpClientTask : public std::enable_shared_from_this<TcpClientTask> {
    public:
        TcpClientTask(const std::string &ip, const uint16_t port);
        ~TcpClientTask();
        
        virtual void BeginProcess(EventLoop* worker_loop, EventLoop* network_loop);
        virtual void StopProcess();
        
        void SetMaxReconnectTimes(int count);
        void SetCheckHello(bool enable);
        
        bool SendData(void* data, size_t data_len);
        bool SendDataWithHead(const CopyOnWriteBuffer& buff);
        
        const rtc::Socket::ConnState GetState() const;
        
        virtual void ScheduleEvent(int64_t current_time);
        
        std::string GetRemoteAddress();
        
        CC_SYNTHESIZE(uint64_t, conn_id_, _conn_id)
        
    protected:
        virtual bool OnRecvVerifyMsg(const char* data, size_t size) = 0;
        virtual void MsgParse(const char* data, size_t size) = 0;
        virtual void OnSendVerifyMsg() {};
        virtual void AddToManager() = 0;
        virtual void RemoveFromManager() = 0;
        virtual bool OnSendHelloMsg() = 0;
        virtual void OnConnectOpen() {};
        virtual void OnConnectError() {};
        void OnRecvHelloMsg();
        
    private:
        bool ConnectToServer();
        void OnInternalPacket(const char* bytes, size_t size);
        void OnConnectEvent();
        void OnCloseEvent();
        void TryReconnectServerInner();
        
    protected:
        EventLoop*                          worker_loop_;
        EventLoop*                          network_loop_;
        
    private:
        std::shared_ptr<TcpSocketWrapper>   tcp_socket_;
        std::unique_ptr<TimerEventWatcher>  reconnect_timer_;
        std::string                         server_ip_;
        uint16_t                            server_port_;
        bool                                verified_;
        bool                                check_hello_;
        int64_t                             last_hello_time_;
        int64_t                             last_schedule_time_;
        int32_t                             max_reconnect_times_;
        int32_t                             connect_round_;
    };
}


#endif /* TcpClientTask_hpp */
