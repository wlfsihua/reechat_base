//
//  RtcLogWrapper.cpp
//   
//
//  Created by raymon_wang on 2017/6/29.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#include "rtc_log_wrapper.hpp"

#include <webrtc/base/logging.h>

namespace reechat {
    rtc::LoggingSeverity RtcLogWrapper::s_min_log_lev = rtc::LS_INFO;
    
    void RtcLogWrapper::SetMinLogLevel(rtc::LoggingSeverity sev)
    {
        s_min_log_lev = sev;
    }
}
