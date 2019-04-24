//
//  TcpServerTask.cpp
//  SignalServer
//
//  Created by raymon_wang on 2018/3/12.
//  Copyright © 2018年 reechat studio. All rights reserved.
//

#include "tcp_server_task.hpp"

#include <webrtc/base/timeutils.h>

#include "tcp_socket_wrapper.hpp"
#include "rtc_log_wrapper.hpp"
#include "string_helper.h"

using namespace std::placeholders;

namespace reechat {
    TcpServerTask::TcpServerTask() :
    verified_client_(false),
    last_hello_time_(0)
    {
        
    }
    
    TcpServerTask::~TcpServerTask()
    {
        if (conn_socket_) {
            conn_socket_->CloseConnection();
            conn_socket_ = nullptr;
        }
    }
    
    bool TcpServerTask::BeginProcess(std::shared_ptr<TcpSocketWrapper> socket)
    {
        if (!socket) {
            return false;
        }
        conn_socket_ = socket;
        socket->SignalReadPacket = std::bind(&TcpServerTask::OnInternalPacket, shared_from_this(), _1, _2);
        socket->SignalOnCloseEvent = std::bind(&TcpServerTask::OnCloseEvent, shared_from_this());
        
        auto worker_loop = socket->WorkerLoop();
        close_timer_.reset(new TimerEventWatcher(worker_loop, std::bind(&TcpServerTask::StopProcess, shared_from_this()), 5));
        close_timer_->Init();
        
        verify_timer_.reset(new TimerEventWatcher(worker_loop, std::bind(&TcpServerTask::OnCheckVerified, shared_from_this()), 3000));
        verify_timer_->Init();
        verify_timer_->AsyncWait();
        
        last_hello_time_ = rtc::TimeMillis();
        
        return true;
    }
    
    void TcpServerTask::StopProcess()
    {
        RemoveFromManager();
        
        if (conn_socket_) {
            conn_socket_->SignalReadPacket = nullptr;
            conn_socket_->SignalOnCloseEvent = nullptr;
            conn_socket_->CloseConnection();
            conn_socket_ = nullptr;
        }
        close_timer_.reset();
        verify_timer_.reset();
    }
    
    void TcpServerTask::StopProcessV2(bool notify_manager)
    {
        if (notify_manager) {
            RemoveFromManager();
        }
        if (conn_socket_) {
            conn_socket_->SignalReadPacket = nullptr;
            conn_socket_->SignalOnCloseEvent = nullptr;
            conn_socket_->CloseConnection();
            conn_socket_ = nullptr;
        }
        close_timer_.reset();
        verify_timer_.reset();
    }
    
    bool TcpServerTask::SendData(const void *data, size_t data_len)
    {
        return conn_socket_ && conn_socket_->GetState() == rtc::Socket::CS_CONNECTED ? conn_socket_->SendData(data, data_len) : false;
    }
    
    void TcpServerTask::ScheduleEvent(int64_t current_time)
    {
        if (last_hello_time_ == 0) {
            return;
        }
        
        int64_t offset_time = current_time - last_hello_time_;
        if (verified_client_ && offset_time > 30000) {
            RTCHAT_LOG(LS_WARNING) << Avar("client (%s) connection timeout, close", GetRemoteAddress().c_str());
            close_timer_->AsyncWait();
        }
    }
    
    std::string TcpServerTask::GetRemoteAddress()
    {
        return conn_socket_ ? conn_socket_->GetRemoteAddress().ToString() : "";
    }
    
    void TcpServerTask::OnRecvHelloMsg()
    {
        last_hello_time_ = rtc::TimeMillis();
    }
    
    void TcpServerTask::OnInternalPacket(const char* bytes, size_t size)
    {
        if (!verified_client_) {
            if (OnRecvVerifyMsg(bytes, size)) {
                verified_client_ = true;
                AddToManager();
            }
        }
        else {
            MsgParse(bytes, size);
        }
    }
    
    void TcpServerTask::OnCloseEvent()
    {
        RTCHAT_LOG(LS_INFO) << "receive close event";
        StopProcess();
    }
    
    void TcpServerTask::OnCheckVerified()
    {
        if (!verified_client_) {
            RTCHAT_LOG(LS_INFO) << "TcpServerTask check verify msg timeout, del it";
            close_timer_->AsyncWait();
        }
    }
}




