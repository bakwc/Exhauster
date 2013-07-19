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

size_t CalcPunctCount(const string& text) {
    size_t count = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (ispunct(text[i])) {
            count++;
        }
    }
    return count;
}

bool HasPunct(const string& text) {
    for (size_t i = 0; i < text.size(); i++) {
        //if (ispunct(text[i])) {
        if (text[i] == '.' ||
                text[i] == ',' ||
                text[i] == '?' ||
                text[i] == '!')
        {
            return true;
        }
    }
    return false;
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

string ImproveText(const string& text) {
    wstring wtext = UTF8ToWide(text);
    wstring current;
    wstring result;

    for (size_t i = 0; i < wtext.size(); i++) {
        if (iswspace(wtext[i])) {
            wtext[i] = ' ';
        } else {
            current += wtext[i];
        }
        if (wtext[i] == ' ' || i == wtext.length() - 1) {
            if (!current.empty() &&
                    current[0] != '.' &&
                    current[0] != ',' &&
                    current[0] != '!' &&
                    current[0] != '?')
            {
                if (!result.empty()) {
                    result += ' ';
                }
            }
            result += current;
            current.clear();
        }
    }
    if (iswupper(result[0]) && !iswpunct(result[result.size() - 1])) {
        result += '.';
    }
    return WideToUTF8(result);
}
