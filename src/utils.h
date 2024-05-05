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


#ifndef ENV_TOOLS_UTILS_H
#define ENV_TOOLS_UTILS_H

#include <iostream>
#include <string>
#include <windows.h>
#include <locale>
#include <codecvt>

std::string ConvertLPWSTRToString(LPWSTR lpwstr);
std::string BSTRToString(BSTR bstr);
wchar_t* StringToWchar(std::string str);
#endif //ENV_TOOLS_UTILS_H
