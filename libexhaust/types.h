#pragma once

#include <list>
#include <vector>
#include <string>
#include <iostream>

#include <contrib/htmlcxx/html/ParserDom.h>

using namespace std;
using namespace htmlcxx;

namespace NExhauster {

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

struct TContentBlock {
    string Title;
    string Text;
    string Snippet;
    vector<string> Urls;
    string Path;
};

struct TSettings {
    TSettings();
    bool Decode; // tries to recode
    string Charset; // optional - if encoding detection fails
                    // will use this charset
    ostream* DebugOutput;
};

typedef list<TElement> TElements;

void DumpTree(const tree<HTML::Node>& dom, ostream& out);
void DumpElements(const TElements& elements, ostream& out);
void DumpBlocks(const vector<TContentBlock>& blocks, ostream& out);

} // NExhauster
