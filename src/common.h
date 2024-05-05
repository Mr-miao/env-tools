//
// Created by ljx26 on 2023-06-14.
//

#ifndef ENV_TOOLS_COMMON_H
#define ENV_TOOLS_COMMON_H

#define ENV_SUCCESS (0)
#define ENV_ERROR (-1)               //加载服务信息出错
#define ENV_NOTFOUND (-2)            //未找到结果
#define ENV_INVALID_PARAM (-4)       //参数错误
#define ENV_STAT_NORMALLY (1)        //状态正常
#define ENV_STAT_ABNORMAL (2)        //状态异常

#include "napi.h"
#include "node.h"

#endif //ENV_TOOLS_COMMON_H