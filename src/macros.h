//
//  macros.h
//   
//
//  Created by raymon_wang on 16/8/16.
//  Copyright © 2016年 reechat studio. All rights reserved.
//

#ifndef macros_h
#define macros_h

#define SAFE_DELETE(p) if(p) {delete p; p = NULL;}

#define SAFE_DELETEARRAY(p) if(p) {delete [] p; p = NULL;}

#define SAFE_SUB(x,y) ((x) > (y) ? (x)-(y) : 0)

#define CC_SYNTHESIZE(varType, varName, funName)\
protected: varType varName;\
public: virtual varType get##funName(void) const { return varName; }\
public: virtual void set##funName(varType var){ varName = var; }

#define CC_SYNTHESIZE_PASS_BY_REF(varType, varName, funName)\
protected: varType varName;\
public: virtual const varType& get##funName(void) const { return varName; }\
public: virtual void set##funName(const varType& var){ varName = var; }

#define CC_SYNTHESIZE_RETAIN(varType, varName, funName)    \
private: varType varName; \
public: virtual varType get##funName(void) const { return varName; } \
public: virtual void set##funName(varType var)   \
{ \
if (varName != var) \
{ \
CC_SAFE_RETAIN(var); \
CC_SAFE_RELEASE(varName); \
varName = var; \
} \
}

#define AutoFunc(prefix, cmdname) void Process##cmdname(const prefix::cmdname* cmd, size_t cmd_len)
#define AutoFuncJson(cmdname) void Process##cmdname(const Json::Value& j_content)

template <typename T>
class CShareThisHelper {
public:
    CShareThisHelper(std::shared_ptr<T> wrapper){
        socket_wrapper = wrapper;
    };
    std::shared_ptr<T> socket_wrapper;
};

#endif /* Macros_h */
