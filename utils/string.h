#pragma once
#include <string>

using namespace std;

wstring UTF8ToWide(const string& text);
string WideToUTF8(const wstring& text);
size_t CalcWordsCount(const string& text);
string NormalizeText(const string& text, bool hard = true);
