//
//  tcp_server_task.hpp
//  SignalServer
//
//  Created by raymon_wang on 2018/3/12.
//  Copyright © 2018年 reechat studio. All rights reserved.
//

#ifndef TcpServerTask_hpp
#define TcpServerTask_hpp

#include <memory>

#include "macros.h"

namespace reechat {
    class TcpSocketWrapper;
    class TimerEventWatcher;
    class EventLoop;
    
    class TcpServerTask : public std::enable_shared_from_this<TcpServerTask>{
    public:
        TcpServerTask();
        ~TcpServerTask();
        
        bool BeginProcess(std::shared_ptr<TcpSocketWrapper> socket);
        void StopProcess();
        void StopProcessV2(bool notify_manager);
        
        bool SendData(const void* data, size_t data_len);
        
        virtual void ScheduleEvent(int64_t current_time);
        
        std::string GetRemoteAddress();
        
        CC_SYNTHESIZE(uint64_t, conn_id_, _conn_id)

    protected:
        virtual bool OnRecvVerifyMsg(const char* data, size_t size) = 0;
        virtual void MsgParse(const char* data, size_t size) = 0;
        virtual void AddToManager() = 0;
        virtual void RemoveFromManager() = 0;
        void OnRecvHelloMsg();
        
    private:
        void OnInternalPacket(const char* bytes, size_t size);
        void OnCloseEvent();
        void OnCheckVerified();
        
    private:
        std::shared_ptr<TcpSocketWrapper>   conn_socket_;
        bool                                verified_client_;
        std::unique_ptr<TimerEventWatcher>  close_timer_;
        std::unique_ptr<TimerEventWatcher>  verify_timer_;
        EventLoop*                          worker_loop_;
        int64_t                             last_hello_time_;
    };
}



#endif /* TcpServerTask_hpp */
