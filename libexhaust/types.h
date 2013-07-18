#pragma once

#include <list>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

enum EType {
    TP_Header = 0x01,
    TP_Text   = 0x02,
    TP_Link   = 0x04
};

struct TElement {
    TElement() { Type = 0; }
    string Text;
    string Path;
    string Url;
    unsigned char Type;
};

struct TBlock {
    string Title;
    string Text;
    string Snippet;
    vector<string> Urls;
    string Path;
};

typedef list<TElement> TElements;

void DumpElements(const TElements& elements, ostream &out);
