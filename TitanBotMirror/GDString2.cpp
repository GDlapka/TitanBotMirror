#include "GDString2.h"

using namespace std;

//преобразование в ascii с потерей
string wide2str(wstring src) {
	string dst(src.length(), ' '); // Make room for characters

	// Copy string to wstring.
	std::copy(src.begin(), src.end(), dst.begin());

	return dst;
}

wstring str2wide(string src) {
	wstring dst(src.length(), L' '); // Make room for characters

									// Copy string to wstring.
	std::copy(src.begin(), src.end(), dst.begin());

	return dst;
}

wstring intToHexWide(int value) {
	std::wstringstream stream;
	stream << std::hex << value;
	std::wstring result(stream.str());
	if (result.size() == 1) result = L'0' + result;
	for (auto & c : result) c = toupper(c);
	return result;
}

string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}