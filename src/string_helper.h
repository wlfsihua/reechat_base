//
//  StringHelper.h
//   
//
//  Created by raymon_wang on 16/7/12.
//  Copyright © 2016年 reechat studio. All rights reserved.
//

#ifndef StringHelper_h
#define StringHelper_h

#include <string>
#include <algorithm>
#include <cctype>
#include <stdarg.h>
#include <string.h>

namespace reechat {
    /**
     * \brief 把字符串根据token转化为多个字符串
     *
     * 下面是使用例子程序：
     *    <pre>
     *    std::list<string> ls;
     *    stringtok (ls, " this  \t is\t\n  a test  ");
     *    for(std::list<string>const_iterator i = ls.begin(); i != ls.end(); ++i)
     *        std::cerr << ':' << (*i) << ":\n";
     *     </pre>
     *
     * \param container 容器，用于存放字符串
     * \param in 输入字符串
     * \param delimiters 分隔符号
     * \param deep 深度，分割的深度，缺省没有限制
     */
    template <typename Container>
    inline void
    stringtok(Container &container, std::string const &in,
              const char * const delimiters = " \t\n",
              const int deep = 0)
    {
        const std::string::size_type len = in.length();
        std::string::size_type i = 0;
        int count = 0;
        
        while(i < len)
        {
            i = in.find_first_not_of (delimiters, i);
            if (i == std::string::npos)
                return;   // nothing left
            
            // find the end of the token
            std::string::size_type j = in.find_first_of (delimiters, i);
            
            count++;
            // push token
            if (j == std::string::npos
                || (deep > 0 && count > deep)) {
                container.push_back (in.substr(i));
                return;
            }
            else
                container.push_back (in.substr(i, j-i));
            
            // set up for next loop
            i = j + 1;
            
        }
    }
    
    inline std::string Avar(const char *pszFmt,...)
    {
        char szBuffer[1024] = {0};
//		memset(szBuffer, 0, sizeof(szBuffer));
        va_list ap;
        va_start(ap,pszFmt);
        vsnprintf(szBuffer,1024,pszFmt,ap);
        va_end(ap);
        return szBuffer;
    };

}


#endif /* StringHelper_h */
