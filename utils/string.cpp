#include "string.h"

#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

wstring UTF8ToWide(const string& text) {
    return boost::locale::conv::to_utf<wchar_t>(text, "UTF-8");
}

string WideToUTF8(const wstring& text) {
    return boost::locale::conv::from_utf<>(text, "UTF-8");
}

size_t CalcWordsCount(const string& text) {
    size_t count = 0;
    wstring wtext = UTF8ToWide(text);
    for (size_t i = 1; i < wtext.size(); i++) {
        if (!iswalpha(wtext[i]) && iswalpha(wtext[i - 1])) {
            count++;
        }
    }
    return count;
}

string NormalizeText(const string& text, bool hard) {
    wstring wtext = UTF8ToWide(text);
    wstring current;
    wstring result;
    for (size_t i = 0; i < wtext.size(); i++) {
        if (iswspace(wtext[i]) || wtext[i] == '<' || wtext[i] == '>') {
            wtext[i] = ' ';
        }
        if ((hard && iswalpha(wtext[i])) ||
                (!hard && (iswalpha(wtext[i]) ||
                           iswdigit(wtext[i]) ||
                           wtext[i] == '.' ||
                           wtext[i] == ':')))
        {
            current += towlower(wtext[i]);
        }
        if (wtext[i] == ' ' || i == wtext.length() - 1) {
            if (!current.empty()) {
                if (!result.empty()) {
                    result += ' ';
                }
                result += current;
                current.clear();
            }
        }
    }
    return WideToUTF8(result);
}
