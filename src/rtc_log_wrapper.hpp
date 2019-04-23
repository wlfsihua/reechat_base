//
//  RtcLogWrapper.hpp
//   
//
//  Created by raymon_wang on 2017/6/29.
//  Copyright © 2017年 reechat studio. All rights reserved.
//

#ifndef RtcLogWrapper_hpp
#define RtcLogWrapper_hpp

#include <string.h>
#include <string>

#include <webrtc/base/logging.h>

using namespace rtc;

namespace reechat {
    
#define RTCHAT_SET_MIN_LOG_LEV(sev) reechat::RtcLogWrapper::s_min_log_lev = sev;
#define RTCHAT_LOG(sev) !(sev >= reechat::RtcLogWrapper::s_min_log_lev) ? (void)0 : rtc::LogMessageVoidify() & (sev > LS_INFO ? rtc::LogMessage(__FILE__, __LINE__, (rtc::LoggingSeverity)sev, "reechat").stream() : rtc::LogMessage(nullptr, 0, (rtc::LoggingSeverity)sev, "reechat").stream())
    
    class RtcLogWrapper {
    public:
        static void SetMinLogLevel(rtc::LoggingSeverity sev);
        static rtc::LoggingSeverity  s_min_log_lev;
    };
}


#endif /* RtcLogWrapper_hpp */
