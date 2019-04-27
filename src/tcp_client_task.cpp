//
//  TcpClientTask.cpp
//  SignalServer
//
//  Created by raymon_wang on 2018/3/9.
//  Copyright © 2018年 reechat studio. All rights reserved.
//

#include "tcp_client_task.hpp"

#include <webrtc/base/timeutils.h>

#include "rtc_log_wrapper.hpp"
#include "string_helper.h"

using namespace std::placeholders;

namespace reechat {
    TcpClientTask::TcpClientTask(const std::string &ip, const uint16_t port) :
    server_ip_(ip),
    server_port_(port),
    verified_(false),
    last_hello_time_(0),
    check_hello_(true),
    last_schedule_time_(0),
    max_reconnect_times_(-1),
    connect_round_(0)
    {
        
    }
    
    TcpClientTask::~TcpClientTask()
    {
        RTCHAT_LOG(LS_INFO) << "TcpClientTask destruction!";
        reconnect_timer_.reset();
    }
    
    void TcpClientTask::BeginProcess(EventLoop* worker_loop, EventLoop* network_loop)
    {
        worker_loop_ = worker_loop;
        network_loop_ = network_loop;
        reconnect_timer_.reset(new TimerEventWatcher(worker_loop, std::bind(&TcpClientTask::ConnectToServer, shared_from_this()), 3000));
        reconnect_timer_->Init();
        
        ConnectToServer();
    }
    
    void TcpClientTask::StopProcess()
    {
        RemoveFromManager();
        if (tcp_socket_) {
            tcp_socket_->SignalOnConnected = nullptr;
            tcp_socket_->SignalReadPacket = nullptr;
            tcp_socket_->SignalOnCloseEvent = nullptr;
            tcp_socket_->CloseConnection();
            tcp_socket_ = nullptr;
        }
        reconnect_timer_.reset();
    }
    
    void TcpClientTask::SetMaxReconnectTimes(int count)
    {
        max_reconnect_times_ = count;
    }
    
    void TcpClientTask::SetCheckHello(bool enable)
    {
        check_hello_ = enable;
    }
    
    bool TcpClientTask::SendData(void* data, size_t data_len)
    {
        return tcp_socket_ && tcp_socket_->GetState() == rtc::Socket::CS_CONNECTED ? tcp_socket_->SendData(data, data_len) : false;
    }
    
    bool TcpClientTask::SendDataWithHead(const CopyOnWriteBuffer& buff)
    {
        return tcp_socket_ && tcp_socket_->GetState() == rtc::Socket::CS_CONNECTED ? tcp_socket_->SendDataWithHead(buff) : false;
    }
    
    const rtc::Socket::ConnState TcpClientTask::GetState() const
    {
        return tcp_socket_ ? tcp_socket_->GetState() : rtc::Socket::CS_CLOSED;
    }
    
    void TcpClientTask::ScheduleEvent(int64_t current_time)
    {
        if (last_schedule_time_ == 0) {
            if (OnSendHelloMsg()) {
                last_schedule_time_ = current_time;
            }
        }
        else if(current_time - last_schedule_time_ >= 10000) {
            OnSendHelloMsg();
            last_schedule_time_ = current_time;
        }
        
        if (check_hello_ && last_hello_time_ > 0 && current_time - last_hello_time_ > 30000) {
            RTCHAT_LOG(LS_WARNING) << Avar("server (%s:%u) connection timeout, try to reconnect", server_ip_.c_str(), server_port_);
            TryReconnectServerInner();
        }
    }
    
    std::string TcpClientTask::GetRemoteAddress()
    {
        return tcp_socket_ ? tcp_socket_->GetRemoteAddress().ToString() : "";
    }
    
    bool TcpClientTask::ConnectToServer()
    {
        tcp_socket_.reset(new TcpSocketWrapper(worker_loop_, network_loop_));
        tcp_socket_->SignalOnConnected = std::bind(&TcpClientTask::OnConnectEvent, shared_from_this());
        tcp_socket_->SignalReadPacket = std::bind(&TcpClientTask::OnInternalPacket, shared_from_this(), _1, _2);
        tcp_socket_->SignalOnCloseEvent = std::bind(&TcpClientTask::OnCloseEvent, shared_from_this());
        if (tcp_socket_->BeginConnect(server_ip_.c_str(), server_port_)) {
            return true;
        }
        
        return false;
    }
    
    void TcpClientTask::OnInternalPacket(const char *bytes, size_t size)
    {
        if (!verified_) {
            if (OnRecvVerifyMsg(bytes, size)) {
                RTCHAT_LOG(LS_INFO) << Avar("TcpClientTask to server (%s:%u) verify success", server_ip_.c_str(), server_port_);
                verified_ = true;
                AddToManager();
            }
            else {
                StopProcess();
            }
        }
        else {
            MsgParse(bytes, size);
        }
    }
    
    void TcpClientTask::OnConnectEvent()
    {
        RTCHAT_LOG(LS_INFO) << Avar("server (%s:%u) connect success", server_ip_.c_str(), server_port_);
        OnConnectOpen();
        OnSendVerifyMsg();
        last_hello_time_ = rtc::TimeMillis();
        connect_round_ = 0;
    }
    
    void TcpClientTask::OnCloseEvent()
    {
        LOG(LS_ERROR) << Avar("server (%s:%u) connect failed or disconnected", server_ip_.c_str(), server_port_);
        TryReconnectServerInner();
    }
    
    void TcpClientTask::TryReconnectServerInner()
    {
        RTCHAT_LOG(LS_INFO) << Avar("TryReconnectServerInner, (%s:%u)", server_ip_.c_str(), server_port_);
        if (max_reconnect_times_> 0 && ++connect_round_ >= max_reconnect_times_) {
            OnConnectError();
            StopProcess();
            return;
        }
        if (tcp_socket_) {
            tcp_socket_->CloseConnection();
            tcp_socket_ = nullptr;
        }
        if (reconnect_timer_) {
            reconnect_timer_->AsyncWait();
        }
        last_hello_time_ = 0;
        verified_ = false;
    }
    
    void TcpClientTask::OnRecvHelloMsg()
    {
        last_hello_time_ = rtc::TimeMillis();
    }
}




