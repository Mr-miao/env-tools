// 导入 `envtools` 模块
const serviceTool = require('bindings')('service_tool.node');
const startupTool = require('bindings')('startup_tool.node');
const taskTool = require('bindings')('task_tool.node');
const execTool = require('bindings')('exec_tool.node');

/**
 * 获取服务信息
 * @param exeFilePath
 * @returns {*}
 */
function getServiceInfo(exeFilePath) {
    return serviceTool.getServiceInfo(exeFilePath);
}

/**
 * 停止服务
 * @param serviceName
 * @returns {*}
 */
function stopService(serviceName) {
    return serviceTool.stopService(serviceName);
}

/**
 * 启动服务
 * @param serviceName
 * @returns {*}
 */
function startService(serviceName){
    return serviceTool.startService(serviceName);
}

/**
 * 获取服务状态
 * @param serviceName
 * @returns {*}
 */
function getServiceStatus(serviceName){
    return serviceTool.getServiceStatus(serviceName);
}

/**
 * 获取启动项信息
 * @param exeFilePath
 * @returns {*}
 */
function getStartupInfo(exeFilePath) {
    return startupTool.getStartupInfo(exeFilePath);
}

/**
 * 设置开机启动项的状态
 * @param startupName
 * @param execPath
 * @param enable
 * @returns {*}
 */
function setStartup(startupName, execPath, enable){
    return startupTool.setStartup(startupName, execPath, enable);
}

/**
 * 查询启动项是否存在
 * @param startupName
 * @returns {*}
 */
function startupStatus(startupName){
    return startupTool.isStartupExists(startupName);
}

/**
 * 获取计划任务信息
 * @param exeFilePath
 * @returns {*}
 */
function getTaskInfo(exeFilePath) {
    return taskTool.getTaskInfo(exeFilePath);
}

/**
 * 设置计划任务状态
 * @param taskName
 * @param enable
 * @returns {*}
 */
function enableTaskInfo(taskName, enable) {
    return taskTool.enableTaskInfo(taskName, enable);
}

/**
 * 获取计划任务状态
 * @param taskName
 * @returns {*}
 */
function getTaskStatus(taskName){
    return taskTool.getTaskStatus(taskName);
}

/**
 * 启动可执行程序
 * @param execPath 路径
 * @returns {*}
 */
function startExec(execPath){
    return execTool.startExec(execPath);
}

/**
 * 关闭可执行程序
 * @param execName 可执行程序名
 * @returns {*}
 */
function closeExec(execName){
    return execTool.closeExec(execName);
}

/**
 * 检查可执行程序是否在运行
 * @param execName 可执行程序名
 * @returns {*}
 */
function execIsRunning(execName){
    return execTool.execIsRunning(execName);
}

// 导出对外调用的函数接口
module.exports = {
    getServiceInfo,
    stopService,
    startService,
    getServiceStatus,
    getStartupInfo,
    startupStatus,
    setStartup,
    getTaskInfo,
    getTaskStatus,
    enableTaskInfo,
    startExec,
    closeExec,
    execIsRunning,
};