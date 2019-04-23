//
//  BaseStatus.hpp
//  SignalServer
//
//  Created by raymon_wang on 2017/6/2.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#ifndef BaseStatus_hpp
#define BaseStatus_hpp

#include <atomic>
#include <iostream>

static const char* StatusDesc[7] = {
    "kNull", "kInitializing", "kInitialized", "kStarting", "kRunning", "kStopping", "kStopped"
};

namespace reechat {
    class BaseStatus {
    public:
        enum Status {
            kNull = 0,
            kInitializing = 1,
            kInitialized = 2,
            kStarting = 3,
            kRunning = 4,
            kStopping = 5,
            kStopped = 6,
        };
        
        enum SubStatus {
            kSubStatusNull = 0,
            kStoppingListener = 1,
            kStoppingThreadPool = 2,
        };
        
        std::string StatusToString() const {
            return StatusDesc[status_];
        }
        
        bool IsRunning() const {
            return status_.load() == kRunning;
        }
        
        bool IsStopped() const {
            return status_.load() == kStopped;
        }
        
        bool IsStopping() const {
            return status_.load() == kStopping;
        }
        
    protected:
        std::atomic<Status> status_ = { kNull };
        std::atomic<SubStatus> substatus_ = { kSubStatusNull };
    };
}



#endif /* BaseStatus_hpp */
