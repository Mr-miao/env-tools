# env-tools

env-tools是配合envMaster的一个工具库，用于操作Windows Serveice、启动项、计划任务以及可执行程序。

## 编译
### 环境准备
1. 需要node环境
2. 需要Python环境建议3.8
3. 因为service 及 计划任务用到了Windwos SDK，因此要确保您的开发环境安装了Windwos SDK
### binding.pyp配置
修改binding.gyp中的E:/work/git_repo/env-tools/include/windowsSDK/x64/taskschd.lib为您的本地路径
### 安装node-gyp
``
npm install -g node-gyp
``
### 编译
``
node-gyp configure build  
``
### 调试
在``index.js``中有封装的所有可用接口，调试可以参考如下方式：

``
node --napi-modules
const serviceTool = require('bindings')('service_tool.node');
serviceTool.getServiceInfo("C:\Windows\SysWOW64\vmnetdhcp.exe")
``

### 其他
本人并非专业的C++开发工程师，因此在项目内可能存在bug或不合理的代码架构，如果您有问题或建议可以随时联系我谢谢！