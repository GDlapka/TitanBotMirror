#pragma once

#include <string>
#include <algorithm>
#include <sstream>

//преобразование в ascii с потерей
std::string wide2str(std::wstring _src);

std::wstring str2wide(std::string _src);

std::wstring intToHexWide(int _dec_value);

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);