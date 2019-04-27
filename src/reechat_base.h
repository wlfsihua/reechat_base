//
//  reechat_base.h
//   
//
//  Created by raymon_wang on 16/8/19.
//  Copyright © 2016年 reechat studio. All rights reserved.
//

#ifndef reechat_base_h
#define reechat_base_h

#include <functional>
#include <memory>

#include "singleton.h"
#include "string_helper.h"
#include "rtc_log_wrapper.hpp"
#include "tcp_socket_wrapper.hpp"
#include "tcp_server_task.hpp"
#include "tcp_client_task.hpp"

typedef std::function<void()> P0CallBack;
typedef std::function<void(int)> P1IntCallBack;
typedef std::function<void(const std::string&)> P1StringCallBack;
typedef std::function<void(const char*, int)> P1CharP2IntCallBack;
typedef std::function<void(int, char*, int)> P1IntP2CharP3IntCallBack;
typedef std::function<void(const std::string&, char*, int)> P1StrP2CharP3IntCallBack;
typedef std::function<void(const std::string&, int)> P1StrP2IntCallBack;
typedef std::function<void(bool, const std::string&)> P1BoolP2StrCallBack;

typedef P1StrP2CharP3IntCallBack    StorageWriteCallBack;
typedef P1StrP2IntCallBack          DownloadTaskCallBack;
typedef P1IntCallBack RunFinishedCallBack;

typedef std::function<void(const std::string&)> CallBackFuncStr1;

#endif /* RTChatBase_h */
