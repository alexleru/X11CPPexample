#include "StringUtils.hpp"

#include <windows.h>

#include <iomanip>
#include <sstream>

std::wstring ToWide(const std::string& s) {
    if (s.empty()) return L"";
    const int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring ws(size - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], size);
    return ws;
}

std::string FormatNumber(double value) {
    std::ostringstream ss;
    ss << std::setprecision(10) << value;
    std::string s = ss.str();
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}
