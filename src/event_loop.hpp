//
//  event_loop.hpp
//  SignalServer
//
//  Created by raymon_wang on 2017/6/2.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#ifndef EventLoop_hpp
#define EventLoop_hpp

#include "base_status.hpp"
#include "event_watcher.hpp"

#include <functional>
#include <thread>
#include <vector>
#include <mutex>

namespace reechat {
    class EventLoop : public BaseStatus {
    public:
        typedef std::function<void()> Functor;
        EventLoop();
        explicit EventLoop(struct event_base* base);
        virtual ~EventLoop();
        
        void Run();
        void Stop();
        void Cancel();
        
        void RunInLoop(const Functor& handler);
        void QueueInLoop(const Functor& handler);
        
        struct event_base* event_base() {
            return event_base_;
        }
        bool IsInLoopThread() const {
            return tid_ == std::this_thread::get_id();
        }
        
    private:
        bool Init();
        void StopInLoop();
        void DoPendingFunctors();
        bool IsPendingQueueEmpty();
        
    private:
        struct event_base* event_base_;
        bool create_event_base_myself_;
        std::thread::id tid_;
        std::mutex mutex_;
        // We use this to notify the thread when we put a task into the pending_functors_ queue
        std::shared_ptr<PipeEventWatcher> watcher_;
        std::atomic<bool> notified_;
        std::vector<Functor>* pending_functors_; // @Guarded By mutex_
        std::atomic<int> pending_functor_count_;
    };
}

#endif /* EventLoop_hpp */
