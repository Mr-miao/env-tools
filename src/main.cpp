#include <iostream>
#include <windows.h>

int main() {
    const wchar_t* longPath = L"C:\\Program Files\\Docker\\Docker\\resources\\Docker desktop.exe";

    wchar_t shortPath[MAX_PATH];
    DWORD result = GetShortPathNameW(longPath, shortPath, MAX_PATH);

    if (result == 0) {
        std::cerr << "GetShortPathNameW failed. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    std::wcout << L"Short path: " << shortPath << std::endl;

    return 0;
}
