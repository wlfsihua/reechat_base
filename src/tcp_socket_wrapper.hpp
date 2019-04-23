//
//  EventSocketWarpper.hpp
//  EventTest
//
//  Created by raymon_wang on 2017/4/3.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#ifndef EventSocketWarpper_hpp
#define EventSocketWarpper_hpp

#include <stdio.h>
#include <webrtc/base/sigslot.h>
#include <webrtc/base/socket.h>
#include <webrtc/base/copyonwritebuffer.h>

#include "event_loop.hpp"
#include "socket_buffer.hpp"
#include "macros.h"

using namespace rtc;

struct event;

namespace reechat {
    struct ShareThisHelper;
    class SocketBuffer;
    
    class TcpSocketWrapper : public std::enable_shared_from_this<TcpSocketWrapper> {
    public:
        typedef std::function<void(int)> Functor1;
        typedef std::function<void (const char*, size_t)> t_SignalReadPacket;
        typedef std::function<void (std::shared_ptr<TcpSocketWrapper>)> t_SignalNewConnection;
        typedef std::function<void ()> t_SignalOnCloseEvent;
        typedef std::function<void ()> t_SignalOnConnected;
        typedef std::function<void (std::shared_ptr<TcpSocketWrapper>)> t_SignalOnWriteComplete;
        typedef std::function<void (std::shared_ptr<TcpSocketWrapper>, size_t)> t_SignalHighWaterMark;
        
    public:
        TcpSocketWrapper(EventLoop* worker_loop, EventLoop* network_loop);
        virtual ~TcpSocketWrapper();
        
        //server mode
        bool BeginListen(const char* local_ip, uint16_t port);
        
        //client mode
        bool BeginConnect(const char* ip, uint16_t port);
        
        std::shared_ptr<TcpSocketWrapper> AcceptConnection(EventLoop* network_loop);
        
        // accepted connection process
        void BeginProcess(int client_fd);
        
        // user thread write data to sender buffer
        bool SendData(const void* data, size_t len);
        
        bool SendDataWithHead(const CopyOnWriteBuffer& buff);
        
        void CloseConnection();
        
        const rtc::SocketAddress& GetRemoteAddress();
        
        rtc::Socket::ConnState GetState() const;
        
        EventLoop* WorkerLoop() const {return worker_loop_;};
        
        CC_SYNTHESIZE(uint16_t, listen_port_, _listen_port)
        
    public:
        t_SignalReadPacket SignalReadPacket;
        t_SignalOnWriteComplete SignalOnWriteComplete;
        t_SignalHighWaterMark   SignalHighWaterMark;
        
        //server
        t_SignalNewConnection SignalNewConnection;
        t_SignalOnCloseEvent SignalOnCloseEvent;
        
        //client
        t_SignalOnConnected SignalOnConnected;
        
    private:
        void ConnectServerInNetworkLoop();
        void BeginProcessInNetworkLoop(int client_fd);
        //network thread
        //try write senderbuffer data to network, can only call by internal
        void TryWriteBufferToSocket();
        //network thread
        //try read senderbuffer data to network, can only call by internal
        void TryReadDataFromSocket(); //network thread
        void CloseConnectionInNetworkLoop();
        
        void TryProcessBufferedDataInWorkLoop();
    private:
        static void OnNewConnection(int sockfd, short event, void* arg); //server get a new connection
        static void OnConnection(int sockfd, short event, void* arg); //client async connect event
        bool ClientConnected(); //network thread
        static void OnReadEvent(int sockfd, short event, void* arg);
        void OnReadEvent(int sockfd, short event); //network thread
        static void OnWriteEvent(int sockfd, short event, void* arg);
        void OnWriteEvent(int sockfd, short event); //network thread
        void OnErrorEvent(); //network thread
        void OnDestroySocket(); //worker thread
        
    private:
        int                 socketfd_;
        bool                listen_;
        EventLoop*          network_loop_;
        EventLoop*          worker_loop_;
        std::atomic<bool>   auto_reconnect_ = { true };
        int                 reconnect_interval_;
        int                 connecting_timeout_;
        struct event*       socket_read_event_;
        struct event*       socket_write_event_;
        RingBuffer          sender_buffer_;
        RingBuffer          receiver_buffer_;
        rtc::SocketAddress  remote_addr_;
        bool                attached_;
        size_t              high_water_mark_;
        CShareThisHelper<TcpSocketWrapper>*    share_this_helper_;
        
    private:
        std::atomic<rtc::Socket::ConnState> state_;
    };
    
}


#endif /* EventSocketWarpper_hpp */
