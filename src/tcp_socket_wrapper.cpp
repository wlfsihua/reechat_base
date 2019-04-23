//
//  EventSocketWarpper.cpp
//  EventTest
//
//  Created by raymon_wang on 2017/4/3.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#include "tcp_socket_wrapper.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <unistd.h>

#include <event.h>
#include "rtc_log_wrapper.hpp"
#include "string_helper.h"

namespace reechat {
#define MaxBufferSize 16 * 1024 * 1024
    TcpSocketWrapper::TcpSocketWrapper(EventLoop* worker_loop, EventLoop* network_loop) :
    worker_loop_(worker_loop),
    network_loop_(network_loop),
    listen_(false),
    socketfd_(-1),
    state_(rtc::Socket::CS_CLOSED),
    reconnect_interval_(3000),
    connecting_timeout_(3000),
    listen_port_(0),
    SignalOnConnected(nullptr),
    SignalReadPacket(nullptr),
    SignalOnWriteComplete(nullptr),
    SignalHighWaterMark(nullptr),
    SignalNewConnection(nullptr),
    SignalOnCloseEvent(nullptr),
    high_water_mark_(MaxBufferSize),
    sender_buffer_(MaxBufferSize),
    receiver_buffer_(MaxBufferSize)
    {
        socket_read_event_ = new event;
        memset(socket_read_event_, 0, sizeof(struct event));
        socket_write_event_ = new event;
        memset(socket_write_event_, 0, sizeof(struct event));
    }
    
    TcpSocketWrapper::~TcpSocketWrapper()
    {
        RTCHAT_LOG(LS_INFO) << Avar("TcpSocketWrapper destruction (socket_fd = %u)", socketfd_);
        if (socket_read_event_) {
            event_del(socket_read_event_);
        }
        if (socket_write_event_) {
            event_del(socket_write_event_);
        }
        SAFE_DELETE(socket_read_event_);
        SAFE_DELETE(socket_write_event_);
        if (socketfd_ > 0) {
            EVUTIL_CLOSESOCKET(socketfd_);
        }
    }
    
    //server mode
    bool TcpSocketWrapper::BeginListen(const char* local_ip, uint16_t port)
    {
        socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd_ == -1) {
            RTCHAT_LOG(LS_ERROR) << "create socket error! " << strerror(errno);
            return false;
        }
        
        if (evutil_make_socket_nonblocking(socketfd_) == -1) {
            RTCHAT_LOG(LS_ERROR) << Avar("set fd = %d nonblocking failed!");
            return false;
        }
        
        int flag = 1;
        //设置端口重用
        if (setsockopt(socketfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1) {
            RTCHAT_LOG(LS_ERROR) << "set SO_REUSEADDR attr failed";
            return false;
        }
        
        struct sockaddr_in listen_addr;
        memset(&listen_addr, 0, sizeof(listen_addr));
        listen_addr.sin_family = AF_INET;
        listen_addr.sin_port = htons(port);
        listen_addr.sin_addr.s_addr = inet_addr(local_ip);
        
        int ret = bind(socketfd_, (const struct sockaddr*)&listen_addr, sizeof(struct sockaddr));
        if (ret == -1) {
            RTCHAT_LOG(LS_ERROR) << "socket bind failed" << strerror(errno);
            return false;
        }
        
        ret = listen(socketfd_, 128);
        if (ret == -1) {
            RTCHAT_LOG(LS_ERROR) << "socket listen failed" << strerror(errno);
            return false;
        }
        
        listen_ = true;
        listen_port_ = port;
        
        share_this_helper_ = new CShareThisHelper<TcpSocketWrapper>(shared_from_this());
        
        //register libevent event
        event_set(socket_read_event_, socketfd_, EV_READ|EV_PERSIST, TcpSocketWrapper::OnNewConnection, share_this_helper_);
        event_base_set(network_loop_->event_base(), socket_read_event_);
        event_add(socket_read_event_, nullptr);
        
        SAFE_DELETE(socket_write_event_);
        
        return true;
    }
    
    //client mode
    bool TcpSocketWrapper::BeginConnect(const char* ip, uint16_t port)
    {
        remote_addr_.SetIP(ip);
        remote_addr_.SetPort(port);
        
        share_this_helper_ = new CShareThisHelper<TcpSocketWrapper>(shared_from_this());
        network_loop_->RunInLoop(std::bind(&TcpSocketWrapper::ConnectServerInNetworkLoop, shared_from_this()));
        
        return true;
    }
    
    std::shared_ptr<TcpSocketWrapper> TcpSocketWrapper::AcceptConnection(EventLoop *network_loop)
    {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(struct sockaddr_in);
        int client_fd = ::accept(socketfd_, (struct sockaddr*)&client_addr, &len);
        if (client_fd == -1) {
            if (errno != EAGAIN && errno != EINTR) {
                RTCHAT_LOG(LS_WARNING) << "listener bad accept " << strerror(errno);
            }
            return nullptr;
        }
        
        auto client_socket_wrapper = std::shared_ptr<TcpSocketWrapper>(new TcpSocketWrapper(worker_loop_, network_loop));
        client_socket_wrapper->remote_addr_.FromSockAddr(client_addr);
        client_socket_wrapper->BeginProcess(client_fd);
        
        return client_socket_wrapper;
    }
    
    // accepted connection process
    void TcpSocketWrapper::BeginProcess(int client_fd)
    {
        network_loop_->RunInLoop(std::bind(&TcpSocketWrapper::BeginProcessInNetworkLoop, shared_from_this(), client_fd));
    }
    
    bool TcpSocketWrapper::SendData(const void* data, size_t len)
    {
        if (len > 65537) { //65535 + 2(head_len)
            RTCHAT_LOG(LS_ERROR) << "SendData is too long";
            return false;
        }
        if (sender_buffer_.AvailableWrite() < len + 2) {
            RTCHAT_LOG(LS_WARNING) << "Send buffer is full";
            return false;
        }
        
        void* ptr = const_cast<void*>(data);
        *(uint16_t*)ptr = static_cast<uint16_t>(len - 2);
        sender_buffer_.WriteContent((const char*)data, (uint32_t)len);
        network_loop_->RunInLoop(std::bind(&TcpSocketWrapper::TryWriteBufferToSocket, shared_from_this()));
        return true;
    }
    
    bool TcpSocketWrapper::SendDataWithHead(const CopyOnWriteBuffer& buff)
    {
        sender_buffer_.WriteContent((const char*)buff.data(), (uint32_t)buff.size());
        network_loop_->RunInLoop(std::bind(&TcpSocketWrapper::TryWriteBufferToSocket, shared_from_this()));
        return true;
    }
    
    void TcpSocketWrapper::CloseConnection()
    {
        network_loop_->RunInLoop(std::bind(&TcpSocketWrapper::CloseConnectionInNetworkLoop, shared_from_this()));
    }
    
    const rtc::SocketAddress& TcpSocketWrapper::GetRemoteAddress()
    {
        return remote_addr_;
    }
    
    rtc::Socket::ConnState TcpSocketWrapper::GetState() const
    {
        return state_;
    }
    
    void TcpSocketWrapper::ConnectServerInNetworkLoop()
    {
        int ret = 0;
        socketfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socketfd_ == -1) {
            RTCHAT_LOG(LS_ERROR) << "create socket error! " << strerror(errno);
            OnErrorEvent();
            return;
        }

#ifdef __APPLE__
        int value = 1;
        ret = setsockopt(socketfd_, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value));
        if (ret != 0) {
            if (errno != EINPROGRESS && errno != EINTR) {
                RTCHAT_LOG(LS_ERROR) << "socket connect remote error! " << strerror(errno);
                OnErrorEvent();
            }
        }
#endif
        
        event_set(socket_write_event_, socketfd_, EV_WRITE|EV_TIMEOUT, TcpSocketWrapper::OnConnection, share_this_helper_);
        event_base_set(network_loop_->event_base(), socket_write_event_);
        event_set(socket_read_event_, socketfd_, EV_READ|EV_PERSIST, TcpSocketWrapper::OnReadEvent, share_this_helper_);
        event_base_set(network_loop_->event_base(), socket_read_event_);
        
        struct sockaddr_in remote_addr;
        remote_addr_.ToSockAddr(&remote_addr);
        //        memset(&remote_addr, 0, sizeof(remote_addr));
        //        remote_addr.sin_family = AF_INET;
        //        remote_addr.sin_port = htons(remote_addr);
        //        remote_addr.sin_addr.s_addr = inet_addr(ip);
        
        evutil_make_socket_nonblocking(socketfd_);
        
        ret = ::connect(socketfd_, (struct sockaddr*)&remote_addr, sizeof(struct sockaddr));
        int serrno = errno;
        if (ret != 0) {
            if (serrno != EINPROGRESS && serrno != EINTR) {
                RTCHAT_LOG(LS_ERROR) << "socket connect remote error! " << strerror(serrno);
                OnErrorEvent();
            }
        }
        state_ = rtc::Socket::CS_CONNECTING;
        
        struct timeval tv1;
        tv1.tv_sec = 5;
        tv1.tv_usec = 0;
        event_add(socket_write_event_, &tv1);
    }
    
    void TcpSocketWrapper::BeginProcessInNetworkLoop(int client_fd)
    {
        socketfd_ = client_fd;
        if (evutil_make_socket_nonblocking(socketfd_) == -1) {
            RTCHAT_LOG(LS_ERROR) << Avar("set fd = %d nonblocking failed!");
            OnErrorEvent();
            return;
        }
        
        state_ = rtc::Socket::CS_CONNECTED;
        
        share_this_helper_ = new CShareThisHelper<TcpSocketWrapper>(shared_from_this());
        
        event_set(socket_read_event_, client_fd, EV_READ|EV_PERSIST, TcpSocketWrapper::OnReadEvent, share_this_helper_);
        event_base_set(network_loop_->event_base(), socket_read_event_);
        event_add(socket_read_event_, nullptr);
        
        event_set(socket_write_event_, client_fd, EV_WRITE, TcpSocketWrapper::OnWriteEvent, share_this_helper_);
        event_base_set(network_loop_->event_base(), socket_write_event_);
    }
    
    void TcpSocketWrapper::TryWriteBufferToSocket()
    {
        uint32_t available_read = sender_buffer_.AvailableSeriesRead();
        while (available_read > 0) {
            ssize_t send_n = ::send(socketfd_, sender_buffer_.CurrentSeriesReadHead(), available_read, 0);
            if (send_n < 0) {
                if (errno != EAGAIN && errno != EINTR) {
                    RTCHAT_LOG(LS_ERROR) << "this=" << this << " TCPConn::HandleWrite errno=" << errno << " " << strerror(errno);
                    OnErrorEvent();
                    return;
                }
                else {
                    event_add(socket_write_event_, nullptr);
                    break;
                }
            }
            else {
                sender_buffer_.ConsumeSeriesReadLen((uint32_t)send_n);
            }
            available_read = sender_buffer_.AvailableSeriesRead();
        }
    }
    
    //try read senderbuffer data to network, can only call by internal
    void TcpSocketWrapper::TryReadDataFromSocket()
    {
        uint32_t available_series_write = receiver_buffer_.AvailableSeriesWrite();
        ssize_t read_n = ::read(socketfd_, receiver_buffer_.CurrentSeriesWriteHead(), available_series_write);
        if (read_n == 0) {
            RTCHAT_LOG(LS_WARNING) << Avar("We read 0 bytes and close the socket (socket_fd = %d)", socketfd_);
            OnErrorEvent();
            return;
        }
        else if (read_n == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                RTCHAT_LOG(LS_WARNING) << "errno=" << errno << " " << strerror(errno);
            } else {
                RTCHAT_LOG(LS_ERROR) << "errno=" << errno << " " << strerror(errno) << " We are closing this connection now.";
                OnErrorEvent();
            }
            return;
        }
        else {
            receiver_buffer_.ConsumeSeriesWriteLen((uint32_t)read_n);
        }
        
        worker_loop_->QueueInLoop(std::bind(&TcpSocketWrapper::TryProcessBufferedDataInWorkLoop, shared_from_this()));
    }
    
    void TcpSocketWrapper::CloseConnectionInNetworkLoop()
    {
        struct timeval tm;
        tm.tv_sec = 2;
        tm.tv_usec = 0;
        
        state_ = rtc::Socket::CS_CLOSED;
        if (socket_read_event_) {
            event_del(socket_read_event_);
        }
        if (socket_write_event_) {
            event_del(socket_write_event_);
        }
        worker_loop_->RunInLoop(std::bind(&TcpSocketWrapper::OnDestroySocket, shared_from_this()));
    }
    
    void TcpSocketWrapper::TryProcessBufferedDataInWorkLoop()
    {
        while (true) {
            uint32_t available_read = receiver_buffer_.AvailableRead();
            uint32_t available_series_read = receiver_buffer_.AvailableSeriesRead();
            if (available_read < sizeof(uint16_t)) {
                break;
            }
            
            uint16_t payload_len = 0;
            //读2个字节获取payload长度
            if (available_series_read >= sizeof(uint16_t)) {
                payload_len = *((uint16_t*)receiver_buffer_.CurrentSeriesReadHead());
            }
            else {
                char head_buffer[2];
                memcpy(head_buffer, receiver_buffer_.CurrentSeriesReadHead(), 1);
                memcpy(head_buffer + 1, receiver_buffer_.DataHead(), 1);
                payload_len = *(uint16_t*)head_buffer;
            }
            
            if (payload_len > 65535 || payload_len <= 0) {
                //illegal playload len, close connection
                RTCHAT_LOG(LS_ERROR) << "payload length illegal, close the connection!";
                event_del(socket_read_event_);
                event_del(socket_write_event_);
                if (SignalOnCloseEvent) {
                    SignalOnCloseEvent();
                }
                break;
            }
            
            uint32_t msg_len = payload_len + sizeof(uint16_t);
            if (available_read < msg_len) {
                break;
            }
            
            if (available_series_read >= msg_len) {
                if (SignalReadPacket) {
                    SignalReadPacket(receiver_buffer_.CurrentSeriesReadHead(), msg_len);
                }
                receiver_buffer_.ConsumeSeriesReadLen(msg_len);
            }
            else {
                rtc::Buffer scope_buffer;
                scope_buffer.AppendData(receiver_buffer_.CurrentSeriesReadHead(), available_series_read);
                receiver_buffer_.ConsumeSeriesReadLen(available_series_read);
                uint32_t remain_len = msg_len - available_series_read;
                scope_buffer.AppendData(receiver_buffer_.CurrentSeriesReadHead(), remain_len);
                receiver_buffer_.ConsumeSeriesReadLen(remain_len);
                if (SignalReadPacket) {
                    SignalReadPacket((const char*)scope_buffer.data(), scope_buffer.size());
                }
            }
        }
    }
    
    void TcpSocketWrapper::OnNewConnection(int sockfd, short event, void* arg)
    {
        auto share_this_helper = static_cast<CShareThisHelper<TcpSocketWrapper>*>(arg);
        if (!share_this_helper) {
            return;
        }
        auto tcp_socket_wrapper = share_this_helper->socket_wrapper;
        if (!tcp_socket_wrapper) {
            return;
        }
        if (tcp_socket_wrapper && tcp_socket_wrapper->SignalNewConnection) {
            tcp_socket_wrapper->SignalNewConnection(tcp_socket_wrapper);
        }
    }
    
    void TcpSocketWrapper::OnConnection(int sockfd, short event, void* arg)
    {
        auto share_this_helper = static_cast<CShareThisHelper<TcpSocketWrapper>*>(arg);
        if (!share_this_helper) {
            return;
        }
        auto tcp_socket_wrapper = share_this_helper->socket_wrapper;
        if (!tcp_socket_wrapper) {
            return;
        }
        short what = event;
        if (event == EV_WRITE) {
            int err;
            socklen_t len = sizeof(err);
            getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &len);
            if (err) {
                RTCHAT_LOG(LS_ERROR) << "connect return ev_write, but check failed";
                what |= EVBUFFER_ERROR;
                tcp_socket_wrapper->OnErrorEvent();
            }
            else{
                tcp_socket_wrapper->ClientConnected();
            }
        }
        else if (event == EV_TIMEOUT) {
            tcp_socket_wrapper->OnErrorEvent();
        }
    }
    
    bool TcpSocketWrapper::ClientConnected()
    {
        event_add(socket_read_event_, NULL);
        
        event_set(socket_write_event_, socketfd_, EV_WRITE, TcpSocketWrapper::OnWriteEvent, share_this_helper_);
        event_base_set(network_loop_->event_base(), socket_write_event_);
        
        state_ = rtc::Socket::CS_CONNECTED;
        
        if (SignalOnConnected) {
            worker_loop_->RunInLoop(std::bind(SignalOnConnected));
        }
        return true;
    }
    
    void TcpSocketWrapper::OnReadEvent(int sockfd, short event, void* arg)
    {
        auto share_this_helper = static_cast<CShareThisHelper<TcpSocketWrapper>*>(arg);
        if (!share_this_helper) {
            return;
        }
        auto tcp_socket_wrapper = share_this_helper->socket_wrapper;
        if (!tcp_socket_wrapper) {
            return;
        }
        tcp_socket_wrapper->OnReadEvent(sockfd, event);
    }
    
    void TcpSocketWrapper::OnReadEvent(int sockfd, short event)
    {
        if (state_ == rtc::Socket::CS_CONNECTED) {
            TryReadDataFromSocket();
        }
    }
    
    void TcpSocketWrapper::OnWriteEvent(int sockfd, short event, void* arg)
    {
        auto share_this_helper = static_cast<CShareThisHelper<TcpSocketWrapper>*>(arg);
        if (!share_this_helper) {
            return;
        }
        auto tcp_socket_wrapper = share_this_helper->socket_wrapper;
        if (!tcp_socket_wrapper) {
            return;
        }
        tcp_socket_wrapper->OnWriteEvent(sockfd, event);
    }
    
    void TcpSocketWrapper::OnWriteEvent(int sockfd, short event)
    {
        TryWriteBufferToSocket();
    }
    
    void TcpSocketWrapper::OnErrorEvent()
    {
        event_del(socket_read_event_);
        event_del(socket_write_event_);
        state_ = rtc::Socket::CS_CLOSED;
        
        if (SignalOnCloseEvent) {
            worker_loop_->RunInLoop(std::bind(SignalOnCloseEvent));
        }
    }
    
    void TcpSocketWrapper::OnDestroySocket()
    {
        SAFE_DELETE(share_this_helper_);
    }
}



