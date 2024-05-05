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

#include "utils.h"

std::string ConvertLPWSTRToString(LPWSTR lpwstr){
    std::wstring wstr(lpwstr);
    std::string str(wstr.begin(), wstr.end());
    return str;
}


std::string BSTRToString(BSTR bstr) {
    if (bstr == nullptr) {
        return "";
    }

    int len = ::SysStringLen(bstr); // 获取BSTR的长度
    int size_needed = ::WideCharToMultiByte(CP_UTF8, 0, bstr, len, nullptr, 0, nullptr, nullptr);

    std::string str;
    if (size_needed > 0) {
        str.resize(size_needed);
        ::WideCharToMultiByte(CP_UTF8, 0, bstr, len, &str[0], size_needed, nullptr, nullptr);
    }

    return str;
}

wchar_t* StringToWchar(std::string str){

    int length = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wideStr;
    wideStr.resize(length - 1); // 不包括 NULL 终止字符
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wideStr[0], length);
    // Allocate memory for wchar_t array
    wchar_t* result = new wchar_t[wideStr.length() + 1];
    std::wcscpy(result, wideStr.c_str());

    return result;
}
