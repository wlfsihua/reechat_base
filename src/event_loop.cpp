//
//  EventLoop.cpp
//  SignalServer
//
//  Created by raymon_wang on 2017/6/2.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#include "event_loop.hpp"

#include <assert.h>

#include <event.h>
#include <webrtc/base/logging.h>

#include "rtc_log_wrapper.hpp"

using namespace rtc;

namespace reechat {
    EventLoop::EventLoop() :
    pending_functor_count_(0),
    notified_(false),
    create_event_base_myself_(true)
    {
        event_base_ = event_base_new();
        Init();
    }
    
    EventLoop::EventLoop(struct event_base* base) :
    pending_functor_count_(0),
    notified_(false),
    event_base_(base),
    create_event_base_myself_(false)
    {
        Init();
        
        // When we build an EventLoop instance from an existing event_base
        // object, we will never call EventLoop::Run() method.
        // So we need to watch the task queue here.
        bool rc = watcher_->AsyncWait();
        assert(rc);
        if (!rc) {
            RTCHAT_LOG(LS_ERROR) << "PipeEventWatcher init failed.";
        }
        status_.store(kRunning);
    }
    
    EventLoop::~EventLoop()
    {
        RTCHAT_LOG(LS_VERBOSE) << "EventLoop destruction!";
        watcher_.reset();
        if (event_base_ != nullptr && create_event_base_myself_) {
            event_base_free(event_base_);
            event_base_ = nullptr;
        }
        delete pending_functors_;
        pending_functors_ = nullptr;
    }
    
    void EventLoop::Run()
    {
        status_.store(kStarting);
        tid_ = std::this_thread::get_id(); // The actual thread id
        
        int rc = watcher_->AsyncWait();
        assert(rc);
        if (!rc) {
            RTCHAT_LOG(LS_ERROR) << "PipeEventWatcher init failed.";
        }
        
        // After everything have initialized, we set the status to kRunning
        status_.store(kRunning);
        
        rc = event_base_dispatch(event_base_);
        if (rc == 1) {
            RTCHAT_LOG(LS_ERROR) << "event_base_dispatch error: no event registered";
        } else if (rc == -1) {
            int serrno = errno;
            RTCHAT_LOG(LS_ERROR) << "event_base_dispatch error " << serrno << " " << strerror(serrno);
        }
        
        // Make sure watcher_ does construct, initialize and destruct in the same thread.
        watcher_.reset();
        RTCHAT_LOG(LS_VERBOSE) << "EventLoop stopped, tid=" << std::this_thread::get_id();
        
        status_.store(kStopped);
    }
    
    void EventLoop::Stop()
    {
        assert(status_.load() == kRunning);
        status_.store(kStopping);
        RTCHAT_LOG(LS_VERBOSE) << "EventLoop::Stop";
        QueueInLoop(std::bind(&EventLoop::StopInLoop, this));
    }
    
    void EventLoop::Cancel()
    {
        watcher_.reset();
    }
    
    void EventLoop::RunInLoop(const Functor& functor)
    {
        if (IsRunning() && IsInLoopThread()) {
            functor();
        } else {
            QueueInLoop(functor);
        }
    }
    
    void EventLoop::QueueInLoop(const Functor& cb)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_functors_->emplace_back(cb);
        
        ++pending_functor_count_;
        RTCHAT_LOG(LS_VERBOSE) << "queued a new Functor. pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << pending_functors_->size() << " notified_=" << notified_.load();
        if (!notified_.load()) {
            RTCHAT_LOG(LS_VERBOSE) << "call watcher_->Nofity() notified_.store(true)";
            notified_.store(true);
            if (watcher_) {
                watcher_->Notify();
            } else {
                RTCHAT_LOG(LS_VERBOSE) << "status=" << StatusToString();
                assert(!IsRunning());
            }
        }
        else {
            RTCHAT_LOG(LS_VERBOSE) << "No need to call watcher_->Nofity()";
        }
    }
    
    bool EventLoop::Init()
    {
        status_.store(kInitializing);
        this->pending_functors_ = new std::vector<Functor>();
        
        tid_ = std::this_thread::get_id();
        
        // Initialized task queue watcher
        watcher_.reset(new PipeEventWatcher(this, std::bind(&EventLoop::DoPendingFunctors, this)));
        int rc = watcher_->Init();
        assert(rc);
        if (!rc) {
            RTCHAT_LOG(LS_ERROR) << "PipeEventWatcher init failed.";
        }
        
        status_.store(kInitialized);
        
        return true;
    }
    
    void EventLoop::StopInLoop()
    {
        RTCHAT_LOG(LS_VERBOSE) << "EventLoop is stopping now, tid=" << std::this_thread::get_id();
        assert(status_.load() == kStopping);
        
        auto f = [this]() {
            for (int i = 0;;i++) {
                RTCHAT_LOG(LS_VERBOSE) << "calling DoPendingFunctors index=" << i;
                DoPendingFunctors();
                if (IsPendingQueueEmpty()) {
                    break;
                }
            }
        };
        
        RTCHAT_LOG(LS_VERBOSE) << "before event_base_loopexit, we invoke DoPendingFunctors";
        
        f();
        
        RTCHAT_LOG(LS_VERBOSE) << "start event_base_loopexit";
        event_base_loopexit(event_base_, nullptr);
        RTCHAT_LOG(LS_VERBOSE) << "after event_base_loopexit, we invoke DoPendingFunctors";
        
        f();
        
        RTCHAT_LOG(LS_VERBOSE) << "end of StopInLoop";
    }
    
    void EventLoop::DoPendingFunctors()
    {
        RTCHAT_LOG(LS_VERBOSE) << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << pending_functors_->size() << " notified_=" << notified_.load();
        
        std::vector<Functor> functors;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            notified_.store(false);
            pending_functors_->swap(functors);
            RTCHAT_LOG(LS_VERBOSE) << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << pending_functors_->size() << " notified_=" << notified_.load();
        }
        RTCHAT_LOG(LS_VERBOSE) << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << pending_functors_->size() << " notified_=" << notified_.load();
        for (size_t i = 0; i < functors.size(); ++i) {
            functors[i]();
            --pending_functor_count_;
        }
        RTCHAT_LOG(LS_VERBOSE) << "pending_functor_count_=" << pending_functor_count_ << " PendingQueueSize=" << pending_functors_->size() << " notified_=" << notified_.load();
    }
    
    bool EventLoop::IsPendingQueueEmpty()
    {
        return pending_functors_->empty();
    }
}



