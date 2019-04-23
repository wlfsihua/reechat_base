//
//  event_watcher.hpp
//  SignalServer
//
//  Created by raymon_wang on 2017/6/2.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#ifndef EventWatcher_hpp
#define EventWatcher_hpp

#include <functional>

struct event;
struct event_base;

namespace reechat {
    class EventLoop;
    
    class EventWatcher {
    public:
        typedef std::function<void()> Handler;

        virtual ~EventWatcher();
        
        bool Init();
        
        // @note It MUST be called in the event thread.
        void Cancel();
        
        // @brief :
        // @param[IN] const Handler& cb - The callback which will be called when this event is canceled.
        // @return void -
        void SetCancelCallback(const Handler& cb);
        
        void ClearHandler() { handler_ = Handler(); }
        
        void SetHandler(const Handler& cb) {handler_ = std::move(cb);};
        
    protected:
        // @note It MUST be called in the event thread.
        // @param timeout the maximum amount of time to wait for the event, or 0 to wait forever
        bool Watch(int timeout_ms);
        
        EventWatcher(struct event_base* evbase, const Handler& handler);
        EventWatcher(struct event_base* evbase, Handler&& handler);
        EventWatcher(struct event_base* evbase);
        void Close();
        void FreeEvent();
        
        virtual bool DoInit() = 0;
        virtual void DoClose() {}
        
    protected:
        struct event* event_;
        struct event_base* event_base_;
        bool attached_;
        Handler handler_;
        Handler cancel_callback_;
    };
    
    class PipeEventWatcher : public EventWatcher {
    public:
        PipeEventWatcher(EventLoop* loop, const Handler& handler);
        PipeEventWatcher(EventLoop* loop, Handler&& handler);
        ~PipeEventWatcher();
        
        bool AsyncWait();
        void Notify();
        int wfd() const {        return pipe_[0];    }
        
    private:
        virtual bool DoInit();
        virtual void DoClose();
        static void HandlerFn(int fd, short which, void* v);
        
        int pipe_[2]; // Write to pipe_[0] , Read from pipe_[1]
    };
    
    class TimerEventWatcher : public EventWatcher {
    public:
        TimerEventWatcher(EventLoop* loop, const Handler& handler, int timeout_ms);
        TimerEventWatcher(EventLoop* loop, Handler&& handler, int timeout_ms);
        TimerEventWatcher(EventLoop* loop, int timeout_ms);
        TimerEventWatcher(struct event_base* loop, const Handler& handler, int timeout_ms);
        TimerEventWatcher(struct event_base* loop, Handler&& handler, int timeout_ms);
        
        bool AsyncWait();
        
    private:
        virtual bool DoInit();
        static void HandlerFn(int fd, short which, void* v);
    private:
        int timeout_ms_;
    };
    
    class SignalEventWatcher : public EventWatcher {
    public:
        SignalEventWatcher(int signo, EventLoop* loop, const Handler& handler);
        SignalEventWatcher(int signo, EventLoop* loop, Handler&& handler);
        
        bool AsyncWait();
    private:
        virtual bool DoInit();
        static void HandlerFn(int sn, short which, void* v);
        
        int signo_;
    };

}


#endif /* EventWatcher_hpp */
