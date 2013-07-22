#pragma once
#include <string>
#include <contrib/stuff/entities.h>

using namespace std;

string LoadFile(const string& fileName);
wstring UTF8ToWide(const string& text);
string WideToUTF8(const wstring& text);
string RecodeText(string text, const string& from, const string& to);
size_t CalcWordsCount(const string& text);
size_t CalcPunctCount(const string& text);
bool HasPunct(const string& text);
string NormalizeText(const string& text, bool hard = true);
string ImproveText(const string& text);
string DecodeHtmlEntities(const string &text);
