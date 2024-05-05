/*
 * Copyright (C) 2023 Mr-Miaow <ljx260403530@qq.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <windows.h>
#include <winsvc.h>
#include <iostream>
#include <algorithm>

#include "common.h"
#include "utils.h"

using namespace std;
using namespace v8;
namespace tools
{
    using v8::Context;
    using v8::Exception;
    using v8::Function;
    using v8::FunctionCallbackInfo;
    using v8::Isolate;
    using v8::Local;
    using v8::NewStringType;
//    using v8::Null;
    using v8::Number;
    using v8::Object;
    using v8::String;
    using v8::Value;


    /**
     * 根据可执行程序获取windwos Service 信息
     * @param args
     * @return
     */
    Napi::Object GetServiceInfo(const Napi::CallbackInfo &args)
    {
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString())
        {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string exeFileName = args[0].ToString().Utf8Value();

        SC_HANDLE scmHandle = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
        if (scmHandle == NULL) {
            cout << "Failed to open Service Control Manager.\n" << endl;
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            return result;
        }

        DWORD bufferSize = 0;
        DWORD servicesReturned = 0;
        DWORD resumeHandle = 0;
        EnumServicesStatusEx(
                scmHandle,
                SC_ENUM_PROCESS_INFO,
                SERVICE_WIN32,
                SERVICE_STATE_ALL,
                NULL,
                bufferSize,
                &bufferSize,
                &servicesReturned,
                &resumeHandle,
                NULL
        );

        if (GetLastError() != ERROR_MORE_DATA) {
            printf("Failed to get required buffer size.\n");
            CloseServiceHandle(scmHandle);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            return result;
        }

        ENUM_SERVICE_STATUS_PROCESS* serviceStatus = (ENUM_SERVICE_STATUS_PROCESS*)malloc(bufferSize);
        EnumServicesStatusEx(
                scmHandle,
                SC_ENUM_PROCESS_INFO,
                SERVICE_WIN32,
                SERVICE_STATE_ALL,
                (LPBYTE)serviceStatus,
                bufferSize,
                &bufferSize,
                &servicesReturned,
                &resumeHandle,
                NULL
        );

        //声明json文档
        Napi::Array doc = Napi::Array::New(env);

        int finded = 0;
        for (DWORD i = 0; i < servicesReturned; i++)
        {
            SC_HANDLE serviceHandle = OpenService(scmHandle, serviceStatus[i].lpServiceName, SERVICE_QUERY_CONFIG);
            if (serviceHandle == NULL)
            {
                continue;
            }

            DWORD bufferSize = 0;
            QueryServiceConfig(serviceHandle, NULL, 0, &bufferSize);
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                CloseServiceHandle(serviceHandle);
                continue;
            }

            LPQUERY_SERVICE_CONFIG serviceConfig = (LPQUERY_SERVICE_CONFIG)malloc(bufferSize);
            if (QueryServiceConfig(serviceHandle, serviceConfig, bufferSize, &bufferSize)) {


                // 将 serviceConfig->lpBinaryPathName 转为大写
                std::string binaryPathNameUppercase = serviceConfig->lpBinaryPathName;
                std::transform(binaryPathNameUppercase.begin(), binaryPathNameUppercase.end(),
                               binaryPathNameUppercase.begin(), ::toupper);
                // 将 exeFileName 转为大写
                std::string exeFileNameUppercase = exeFileName;
                std::transform(exeFileNameUppercase.begin(), exeFileNameUppercase.end(),
                               exeFileNameUppercase.begin(), ::toupper);
                cout << exeFileNameUppercase << endl;
                if (binaryPathNameUppercase.find(exeFileNameUppercase) != std::string::npos)
                {
                    //设置json对象的值
                    Napi::Object data = Napi::Object::New(env);
                    char* lpServiceNam = serviceStatus[i].lpServiceName;
                    char* lpDisplayName = serviceStatus[i].lpDisplayName;

                    data.Set(static_cast<napi_value>(Napi::String::New(env, "name")), lpServiceNam);
                    data.Set(static_cast<napi_value>(Napi::String::New(env, "dispalyName")), lpDisplayName);
                    data.Set(static_cast<napi_value>(Napi::String::New(env, "execPath")), serviceConfig->lpBinaryPathName);
                    doc.Set(finded, data);
                    finded++;
                }
            }

            free(serviceConfig);
            CloseServiceHandle(serviceHandle);
        }

        free(serviceStatus);
        CloseServiceHandle(scmHandle);

        //如果未找到服务
        if(doc.Length() < 1)
        {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_NOTFOUND);
        } else{
            //如果找到服务
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), doc);
        }



        return result;
    }

    /**
     * 根据服务名称停止服务
     * @param args
     * @return
     */
    Napi::Object StopService(const Napi::CallbackInfo &args){
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString())
        {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string serviceNameStr = args[0].ToString().Utf8Value();
        const wchar_t *serviceName = StringToWchar(serviceNameStr);


        SC_HANDLE scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
        if (scm == nullptr) {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open service control manager.Error code: " + std::to_string(error));
            return result;
        }

        SC_HANDLE service = OpenServiceW(scm, serviceName, SERVICE_ALL_ACCESS);
        if (service == nullptr) {

            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open service: " + serviceNameStr + ". Error code: " + std::to_string(error));
            CloseServiceHandle(scm);
            return result;
        }

        SERVICE_STATUS status;
        if (QueryServiceStatus(service, &status)) {
            if (status.dwCurrentState == SERVICE_RUNNING) {
                if (ControlService(service, SERVICE_CONTROL_STOP, &status)) {
                    std::cout << "Service is stopping..." << std::endl;

                    //等待服务关闭
                    while (true){
                        if (QueryServiceStatus(service, &status)) {
                            if (status.dwCurrentState == SERVICE_RUNNING) {
                                Sleep(1000);
                            } else{
                                break;
                            }
                        }
                    }

                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Service is stoped.");
                } else {
                    DWORD error = GetLastError();
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to stop service. Error code: " + std::to_string(error));
                }
            } else {
                std::cerr << "Service is not running." << std::endl;
                result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
                result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Service is not running.");
            }
        } else {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to query service status. Error code: " + std::to_string(error));
        }

        //关停服务后，将服务设置为手动
        if (!ChangeServiceConfig(
                service,
                SERVICE_NO_CHANGE,  // 服务类型
                SERVICE_DEMAND_START, // 启动类型：手动
                SERVICE_NO_CHANGE, // 错误控制类型
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr)) {
            DWORD error = GetLastError();
            std::cerr << "Failed to change service configuration. err:" << std::to_string(error) << std::endl;
        }

        CloseServiceHandle(service);
        CloseServiceHandle(scm);

        return result;
    }


    /**
     * 根据服务名称动服务
     * @param args
     * @return
     */
    Napi::Object StartService(const Napi::CallbackInfo &args){
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString())
        {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string serviceNameStr = args[0].ToString().Utf8Value();
        const wchar_t *serviceName = StringToWchar(serviceNameStr);

        SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (scm == nullptr) {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open service control manager.Error code: " + std::to_string(error));
            return result;
        }

        SC_HANDLE service = OpenServiceW(scm, serviceName, SERVICE_START);
        if (service == nullptr) {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open service: " + serviceNameStr + ". Error code: " + std::to_string(error));
            CloseServiceHandle(scm);
            return result;
        }

        if (!StartService(service, 0, nullptr)) {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to start service:" + serviceNameStr + ". Error code: " + std::to_string(error));
        }else{
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Service is started.");
        }

        CloseServiceHandle(service);
        CloseServiceHandle(scm);

        return result;
    }

    /**
     * 查询windows Service状态
     * @param args
     * @return
     */
    Napi::Object GetServiceStatus(const Napi::CallbackInfo &args) {
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString()) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string serviceNameStr = args[0].ToString().Utf8Value();
        const wchar_t *serviceName = StringToWchar(serviceNameStr);

        SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scm) {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open service control manager.Error code: " + std::to_string(error));
            return result;
        }

        SC_HANDLE service = OpenServiceW(scm, serviceName, SERVICE_QUERY_STATUS);
        if (!service) {
            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open service: " + serviceNameStr + ". Error code: " + std::to_string(error));
            CloseServiceHandle(scm);
            return result;
        }

        SERVICE_STATUS status;
        if (!QueryServiceStatus(service, &status)) {

            DWORD error = GetLastError();
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to query service status.Error code: " + std::to_string(error));

            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            return result;

        }

        DWORD serviceStatus = status.dwCurrentState;

        string serviceStatusStr = "";
        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_ABNORMAL);
        if (serviceStatus == SERVICE_RUNNING) {
            serviceStatusStr = "RUNNING";
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_NORMALLY);
        } else if (serviceStatus == SERVICE_STOPPED) {
            serviceStatusStr = "STOPPED";
        } else if (serviceStatus == SERVICE_PAUSED) {
            serviceStatusStr = "PAUSED";
        } else {
            serviceStatusStr = to_string(serviceStatus);
        }


        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), serviceStatusStr);

        CloseServiceHandle(service);
        CloseServiceHandle(scm);

        return result;
    }


    Napi::Object Init(Napi::Env env, Napi::Object exports) {

      exports.Set("getServiceInfo", Napi::Function::New(env, GetServiceInfo));
      exports.Set("stopService", Napi::Function::New(env, StopService));
      exports.Set("startService", Napi::Function::New(env, StartService));
      exports.Set("getServiceStatus", Napi::Function::New(env, GetServiceStatus));

      return exports;
    }

    NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)


}
