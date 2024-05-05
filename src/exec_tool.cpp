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
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>

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
     * 执行可执行程序
     * @param args
     * @return
     */
    Napi::Object StartExec(const Napi::CallbackInfo &args)
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
        string exeFilePath = args[0].ToString().Utf8Value();


        // 使用system函数执行exe文件
        // 创建进程的结构体
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi;

        // 尝试创建新进程
        if (CreateProcess(NULL, const_cast<LPTSTR>(exeFilePath.c_str()), NULL, NULL, FALSE, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi)) {

            // 等待进程启动
            DWORD waitResult = WaitForInputIdle(pi.hProcess, INFINITE);
            if (waitResult == 0) {
                std::cout << "Process has started." << std::endl;
            } else if (waitResult == WAIT_FAILED) {
                result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "WaitForInputIdle failed. Error code: " + GetLastError());
                return result;
            } else {
                result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Unexpected wait result: " + waitResult);
                return result;
            }

            // 执行成功
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Executable file executed successfully.\n");

            // 关闭进程和线程的句柄
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);


        } else {
            // 执行失败
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error executing executable file: " + GetLastError());
        }


        return result;
    }


    /**
     * 根据进程名称关闭进程
     * @param args 进程名称字符串
     * @return
     */
    Napi::Object CloseExec(const Napi::CallbackInfo &args)
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
        string processName = args[0].ToString().Utf8Value();

        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;

        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {

            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error creating process snapshot. Error code:" + GetLastError());

            return result;
        }

        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error getting first process entry. Error code:" + GetLastError());
            return result;
        }

        do {
            // 将窄字符转换为宽字符
            std::wstring exeFile(pe32.szExeFile, pe32.szExeFile + strlen(pe32.szExeFile));

            if (_wcsicmp(exeFile.c_str(), StringToWchar(processName)) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    if (TerminateProcess(hProcess, 0)) {
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Process terminated successfully.\n");
                    } else {
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error terminating process. Error code:" + GetLastError());
                    }

                    CloseHandle(hProcess);
                } else {
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error opening process. Error code:" + GetLastError());
                }

                CloseHandle(hProcessSnap);
                return result;
            }
        }while (Process32Next(hProcessSnap, &pe32));

        CloseHandle(hProcessSnap);

        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_NOTFOUND);
        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Process not found.");

        return result;
    }

    Napi::Object ExecIsRunning(const Napi::CallbackInfo &args)
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
        string processName = args[0].ToString().Utf8Value();

        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;

        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error creating process snapshot. Error code:" + GetLastError());
            return result;
        }

        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (!Process32First(hProcessSnap, &pe32)) {
            CloseHandle(hProcessSnap);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Error getting first process entry. Error code:" + GetLastError());
            return result;
        }

        do {
            // 将进程名转换为小写，以便不区分大小写进行比较
            transform(processName.begin(),processName.end(),processName.begin(),::tolower);
            std::string lowerExeFile(pe32.szExeFile);
            std::transform(lowerExeFile.begin(), lowerExeFile.end(), lowerExeFile.begin(), ::towlower);

            if (lowerExeFile == processName) {
                CloseHandle(hProcessSnap);

                result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_NORMALLY);
                result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Process is running.\n");
                return result;

            }
        } while (Process32Next(hProcessSnap, &pe32));

        CloseHandle(hProcessSnap);
        result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_ABNORMAL);
        result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Process not found.");

        return result;
    }

    Napi::Object Init(Napi::Env env, Napi::Object exports) {

        exports.Set("startExec", Napi::Function::New(env, StartExec));
        exports.Set("closeExec", Napi::Function::New(env, CloseExec));
        exports.Set("execIsRunning", Napi::Function::New(env, ExecIsRunning));

        return exports;
    }

    NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)


}
