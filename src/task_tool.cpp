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

#include <iostream>
#include <Windows.h>
#include <taskschd.h>
#include <comdef.h>

#include "common.h"
#include "utils.h"

// 定义一个宏用于转换HRESULT为字符串
#define HR_TO_STRING(hr) std::to_string(static_cast<long>(hr))

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
     * 获取开机启动项
     * @param env
     * @param hKey
     * @param execName
     */
    Napi::Object GetTaskInfo(const Napi::CallbackInfo &args)
    {

        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        Napi::Array dataArr = Napi::Array::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString())
        {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "invalid param!");
//            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string exeFileName = args[0].ToString().Utf8Value();

        HRESULT hr;
        CoInitialize(NULL);

        ITaskService *pService = nullptr;
        ITaskFolder *pRootFolder = nullptr;
        IRegisteredTaskCollection *pTaskCollection = nullptr;

        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to create TaskService instance: " + HR_TO_STRING(hr));
            CoUninitialize();
            return result;
        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to connect to TaskService: " + HR_TO_STRING(hr));
            pService->Release();
            CoUninitialize();
            return result;
        }

        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to get root folder: " + HR_TO_STRING(hr));
            pService->Release();
            CoUninitialize();
            return result;
        }

        hr = pRootFolder->GetTasks(NULL, &pTaskCollection);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to get task collection: " + HR_TO_STRING(hr));
            pRootFolder->Release();
            pService->Release();
            CoUninitialize();
            return result;
        }

        LONG taskCount;
        pTaskCollection->get_Count(&taskCount);

        IRegisteredTask *pTask = nullptr;
        for (LONG i = 0; i < taskCount; i++) {
            hr = pTaskCollection->get_Item(_variant_t(i + 1), &pTask);
            if (FAILED(hr)) {
                result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to get task(Total tasks found:" + std::to_string(taskCount) + ") #" + std::to_string(i + 1) );
                continue;
            }

            //创建任务信息json对象
            Napi::Object taskItem = Napi::Object::New(env);
            string taskDetails = "";

            //获取任务名称
            BSTR taskName;
            pTask->get_Name(&taskName);
            taskItem.Set(static_cast<napi_value>(Napi::String::New(env, "name")), BSTRToString(taskName));
            SysFreeString(taskName);

            ITaskDefinition *pDefinition = nullptr;
            hr = pTask->get_Definition(&pDefinition);
            if (SUCCEEDED(hr)) {
                IActionCollection *pActionCollection = nullptr;
                hr = pDefinition->get_Actions(&pActionCollection);
                if (SUCCEEDED(hr)) {
                    LONG actionCount;
                    pActionCollection->get_Count(&actionCount);
//                    std::cout << "Action Count: " << actionCount << std::endl;

                    for (LONG j = 0; j < actionCount; j++) {
                        IAction *pAction = nullptr;
                        hr = pActionCollection->get_Item(j + 1, &pAction);
                        if (FAILED(hr)) {
                            std::cerr << "Failed to get action #" << (j + 1) << std::endl;
                            continue;
                        }

                        TASK_ACTION_TYPE actionType;
                        pAction->get_Type(&actionType);
                        if (actionType == TASK_ACTION_EXEC) {
                            IExecAction *pExecAction = nullptr;
                            hr = pAction->QueryInterface(IID_IExecAction, (void**)&pExecAction);
                            if (SUCCEEDED(hr)) {
                                BSTR execPath;
                                hr = pExecAction->get_Path(&execPath);
                                //查看当前任务是否与传入的可执行文件有关系
                                if (SUCCEEDED(hr) && BSTRToString(execPath).compare(exeFileName) == 0) {
                                    taskDetails += "Action #"
                                            + std::to_string(actionCount)
                                            + BSTRToString(execPath)
                                            + "\r\n";
                                    SysFreeString(execPath);
                                }
                                pExecAction->Release();
                            }
                        }

                        pAction->Release();
                    }

                    pActionCollection->Release();
                }


                pDefinition->Release();
            }

            //如果taskActions字符串不为空，那么就认为找到了与传入可执行程序相关的计划任务
            if(taskDetails.compare("") != 0){
                taskItem.Set(static_cast<napi_value>(Napi::String::New(env, "details")), taskDetails);
                dataArr.Set(dataArr.Length(), taskItem);
            }

            pTask->Release();
        }

        pTaskCollection->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();

        if(dataArr.Length() > 0){
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), dataArr);
        } else{
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_NOTFOUND);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "not found data");
        }

        return result;
    }

    /**
     * 设置计划任务的状态
     * @param args
     * @return
     */
    Napi::Object EnableTaskInfo(const Napi::CallbackInfo &args) {

        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        Napi::Array dataArr = Napi::Array::New(env);

        //参数错误
        if (args.Length() < 2 || !args[0].IsString() || !args[1].IsBoolean()) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "invalid param!");
            return result;
        }

        //获取参数
        string taskNameStr = args[0].ToString().Utf8Value();
        bool enable = args[1].ToBoolean().Value();
        bool isSuccess = false;


        HRESULT hr;
        CoInitialize(NULL);

        ITaskService *pService = nullptr;
        ITaskFolder *pRootFolder = nullptr;
        IRegisteredTaskCollection *pTaskCollection = nullptr;

        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not creat TaskService");
            CoUninitialize();
            return result;

        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not connect TaskService");
            pService->Release();
            CoUninitialize();
            return result;
        }

        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not get root folder");
            pService->Release();
            CoUninitialize();
            return result;
        }

        hr = pRootFolder->GetTasks(NULL, &pTaskCollection);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not get task collection");
            pRootFolder->Release();
            pService->Release();
            CoUninitialize();
            return result;
        }

        BSTR taskNameToDisable = SysAllocString(StringToWchar(taskNameStr));
        IRegisteredTask *pTask = nullptr;

        //获取所有任务数
        LONG taskCount;
        pTaskCollection->get_Count(&taskCount);

        //遍历任务
        for (LONG i = 0; i < taskCount; i++) {
            hr = pTaskCollection->get_Item(_variant_t(i + 1), &pTask);
            if (FAILED(hr)) {
                std::wcout << "err: " << hr << std::endl;
                continue;
            }

            BSTR taskName;
            pTask->get_Name(&taskName);
            //进行任务名对比，匹配上才做状态更改
            if (wcscmp(taskName, taskNameToDisable) == 0) {

                if(enable){
                    pTask->put_Enabled(VARIANT_TRUE);

                    //启动计划任务不做任务的立即执行
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "setted");

                } else{
                    hr = pTask->Stop(0);

                    if (FAILED(hr)) {
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "stask can not stop!");
                    } else{
                        pTask->put_Enabled(VARIANT_FALSE);

                        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "setted");
                    }

                }

                isSuccess = true;
            }


            SysFreeString(taskName);
            pTask->Release();
        }

        SysFreeString(taskNameToDisable);
        pTaskCollection->Release();
        pRootFolder->Release();
        pService->Release();

        CoUninitialize();

        if(!isSuccess){
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "set failed: could not open task Item or not found " + taskNameStr);
        }

        return result;
    }

    /**
     * 计划任务状态查询
     * @param args
     * @return
     */
    Napi::Object TaskStatus(const Napi::CallbackInfo &args) {

        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString()) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "invalid param!");
            return result;
        }

        //获取参数
        string taskNameStr = args[0].ToString().Utf8Value();
        bool isFounded = false;

        
        HRESULT hr;
        CoInitialize(NULL);

        ITaskService *pService = nullptr;
        ITaskFolder *pRootFolder = nullptr;
        IRegisteredTaskCollection *pTaskCollection = nullptr;

        hr = CoCreateInstance(CLSID_TaskScheduler, NULL, CLSCTX_INPROC_SERVER, IID_ITaskService, (void**)&pService);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not creat TaskService");
            CoUninitialize();
            return result;

        }

        hr = pService->Connect(_variant_t(), _variant_t(), _variant_t(), _variant_t());
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not connect TaskService");
            pService->Release();
            CoUninitialize();
            return result;
        }

        hr = pService->GetFolder(_bstr_t(L"\\"), &pRootFolder);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not get root folder");
            pService->Release();
            CoUninitialize();
            return result;
        }

        hr = pRootFolder->GetTasks(NULL, &pTaskCollection);
        if (FAILED(hr)) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Could not get task collection");
            pRootFolder->Release();
            pService->Release();
            CoUninitialize();
            return result;
        }

        BSTR taskNameToFound = SysAllocString(StringToWchar(taskNameStr));
        IRegisteredTask *pTask = nullptr;

        //获取所有任务数
        LONG taskCount;
        pTaskCollection->get_Count(&taskCount);

        //遍历任务
        for (LONG i = 0; i < taskCount; i++) {
            hr = pTaskCollection->get_Item(_variant_t(i + 1), &pTask);
            if (FAILED(hr)) {
                std::wcout << "err: " << hr << std::endl;
                continue;
            }

            BSTR taskName;
            pTask->get_Name(&taskName);
            //进行任务名对比，匹配上才做状态查询
            if (wcscmp(taskName, taskNameToFound) == 0) {
                TASK_STATE taskState;
                hr = pTask->get_State(&taskState);
                if (SUCCEEDED(hr)) {
                    if(taskState == TASK_STATE_DISABLED){
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_ABNORMAL);
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "disabled");
                    } else{
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_NORMALLY);
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "enable");
                    }
                } else{
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "task state get error");
                }

                isFounded = true;

            }


            SysFreeString(taskName);
            pTask->Release();

            if(isFounded){
                break;
            }
        }

        SysFreeString(taskNameToFound);
        pTaskCollection->Release();
        pRootFolder->Release();
        pService->Release();
        CoUninitialize();

        if(!isFounded){
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_NORMALLY);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "set failed: could not open task Item or not found " + taskNameStr);
        }

        return result;


    }


    Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        exports.Set("getTaskInfo", Napi::Function::New(env, GetTaskInfo));
        exports.Set("enableTaskInfo", Napi::Function::New(env, EnableTaskInfo));
        exports.Set("getTaskStatus", Napi::Function::New(env, TaskStatus));


        return exports;
    }


    NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
}