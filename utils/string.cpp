#include <fstream>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>
#include <iconv.h>

#include "string.h"

string LoadFile(const string& fileName) {
    std::ifstream ifs(fileName);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    return content;
}

wstring UTF8ToWide(const string& text) {
    return boost::locale::conv::to_utf<wchar_t>(text, "UTF-8");
}

string WideToUTF8(const wstring& text) {
    return boost::locale::conv::from_utf<>(text, "UTF-8");
}

string RecodeCharset(string text, const string& from, const string& to) {
    iconv_t cnv = iconv_open(to.c_str(), from.c_str());
    if (cnv == (iconv_t)-1) {
        iconv_close(cnv);
        return "";
    }

    char* outbuf;
    if ((outbuf = (char*)malloc(text.length() * 2 + 1)) == NULL) {
        iconv_close(cnv);
        return "";
    }

    char* ip = (char*)text.c_str();
    char* op = outbuf;
    size_t icount = text.length();
    size_t ocount = text.length() * 2;

    if (iconv(cnv, &ip, &icount, &op, &ocount) != (size_t)-1) {
        outbuf[text.length() * 2 - ocount] = '\0';
        text = outbuf;
    } else {
        text = "";
    }

    free(outbuf);
    iconv_close(cnv);
    return text;
}

size_t CalcWordsCount(const string& text) {
    size_t count = 0;
    wstring wtext = UTF8ToWide(text + " ");
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
        if (iswspace(wtext[i]) ||
                wtext[i] == '<' ||
                wtext[i] == '>' ||
                (hard && wtext[i] == '.'))
        {
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
