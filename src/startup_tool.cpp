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
     * 获取开机启动项
     * @param env
     * @param hKey
     * @param execName
     */
    void QueryStartupItem(Napi::Env env, Napi::Array dataArr, HKEY hKey, string execPath)
    {
        LPCWSTR keyPath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
        if (RegOpenKeyExW(hKey, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            DWORD maxValueNameSize = 1024;
            LPWSTR valueName = new WCHAR[maxValueNameSize];
            DWORD valueNameSize = maxValueNameSize;
            DWORD valueType;
            BYTE valueData[1024];
            DWORD valueDataSize = sizeof(valueData);

            // 枚举注册表值
            for (DWORD i = 0; RegEnumValueW(hKey, i, valueName, &valueNameSize, NULL, &valueType, valueData, &valueDataSize) == ERROR_SUCCESS; i++)
            {
                string itemPathStr = ConvertLPWSTRToString(reinterpret_cast<LPWSTR>(valueData));

                //如果注册表键值是字符类型，且键值中的数据项和传入的待着搜索可执行路径一致
                if (valueType == REG_SZ && strstr(itemPathStr.c_str(), execPath.c_str()) != NULL)
                {
                    Napi::String itemName = Napi::String::New(env,ConvertLPWSTRToString(valueName));
                    Napi::String itemPath = Napi::String::New(env,itemPathStr);

                    // 保存启动项名称和路径信息
                    Napi::Object startupItem = Napi::Object::New(env);
                    startupItem.Set(static_cast<napi_value>(Napi::String::New(env, "name")), itemName);
                    startupItem.Set(static_cast<napi_value>(Napi::String::New(env, "path")), itemPath);
                    dataArr.Set(dataArr.Length(), startupItem);
                }

                // 重置缓冲区和大小
                valueNameSize = maxValueNameSize;
                valueDataSize = sizeof(valueData);
            }

            delete[] valueName;
            RegCloseKey(hKey);
        }


//        return startupItem;
    }



    Napi::Object GetStartupInfo(const Napi::CallbackInfo &args){
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

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


        Napi::Array dataArr = Napi::Array::New(env);

        //查询本地机器的开机启动项
        HKEY localMachineKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, NULL, 0, KEY_READ, &localMachineKey) == ERROR_SUCCESS)
        {
            QueryStartupItem(env, dataArr, localMachineKey, exeFileName);
        }

        //查询当前用户的开机启动项
        HKEY currentUserKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, NULL, 0, KEY_READ, &currentUserKey) == ERROR_SUCCESS)
        {
            QueryStartupItem(env, dataArr, currentUserKey, exeFileName);
        }

        if(dataArr.Length() < 1){
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_NOTFOUND);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "can not found startup items");
        }else{
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), dataArr);
        };



        return result;
    }


    Napi::Object SetStartup(const Napi::CallbackInfo &args) {
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 3 || !args[0].IsString() || !args[1].IsString() || !args[2].IsBoolean()) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "invalid param!");
//            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string startupNanme = args[0].ToString().Utf8Value();
        string exePath = args[1].ToString().Utf8Value();
        bool enable = args[2].ToBoolean().Value();


        HKEY hKey;
        LSTATUS regKey = RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE | KEY_READ, &hKey);
        if (regKey != ERROR_SUCCESS) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to open registry key:" + to_string(regKey));
            return result;
        }

        if (enable) {
            regKey = RegSetValueExA(hKey, startupNanme.c_str(), 0, REG_SZ, (BYTE*)exePath.c_str(), (exePath.size() + 1));
        } else {
            regKey = RegDeleteValueA(hKey, startupNanme.c_str());
        }


        if (regKey != ERROR_SUCCESS) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_ERROR);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "Failed to set registry value:" + to_string(regKey));
        } else{
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_SUCCESS);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "success");
        }

        RegCloseKey(hKey);
        return result;

    }

    Napi::Object IsStartupExists(const Napi::CallbackInfo &args) {
        //接口返回对象，每个接口错误或异常都会返回一个object
        Napi::Env env = args.Env();
        Napi::Object result = Napi::Object::New(env);

        //参数错误
        if (args.Length() < 1 || !args[0].IsString()) {
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_INVALID_PARAM);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "invalid param!");
//            cout << "invalid param!\n" << endl;
            return result;
        }

        //获取参数
        string startupNanme = args[0].ToString().Utf8Value();
        HKEY hKey;

        if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD valueType, dataSize = MAX_PATH;
            wchar_t exePath[MAX_PATH];

            if (RegQueryValueExA(hKey, startupNanme.c_str(), nullptr, &valueType, reinterpret_cast<LPBYTE>(exePath), &dataSize) == ERROR_SUCCESS) {
                if (valueType == REG_SZ) {
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_NORMALLY);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "success");
                } else{
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_NOTFOUND);
                    result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "can not found startup items");
                }
            } else{
                result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_NOTFOUND);
                result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "can not found startup items");
            }

            RegCloseKey(hKey);
        } else{
            result.Set(static_cast<napi_value>(Napi::String::New(env, "retCode")), ENV_STAT_NORMALLY);
            result.Set(static_cast<napi_value>(Napi::String::New(env, "data")), "can not found startup items");
        }

        return result;
    }


    Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        exports.Set("getStartupInfo", Napi::Function::New(env, GetStartupInfo));
        exports.Set("setStartup", Napi::Function::New(env, SetStartup));
        exports.Set("isStartupExists", Napi::Function::New(env, IsStartupExists));

        return exports;
    }


    NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
}
