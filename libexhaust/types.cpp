#include "types.h"

namespace NExhauster {

TSettings::TSettings()
    : Decode(true)
    , DebugOutput(NULL)
{
}

std::string ETypeToString(unsigned char type) {
    std::string result;
    if (type & TP_Text) {
        result += "TP_Text, ";
    }
    if (type & TP_Header) {
        result += "TP_Header, ";
    }
    if (type & TP_Link) {
        result += "TP_Link, ";
    }

    if (result.size() > 0) {
        result.erase(result.size() - 2);
    } else {
        result = "TP_Unknown";
    }
    return result;
}

// todo: formated output
void DumpTree(const tree<HTML::Node>& dom, ostream& out) {
    tree<HTML::Node>::iterator it = dom.begin();
    out << it->text();
    for (unsigned i = 0; i < dom.number_of_children(it); i++) {
        DumpTree(dom.child(it, i), out);
    }
    out << it->closingText();
}

void DumpElements(const TElements& elements, ostream& out) {
    out << "elements size: " << elements.size() << "\n";
    size_t elementNum = 0;
    for (TElements::const_iterator it = elements.begin(); it != elements.end(); it++) {
        out << "element: " << elementNum << "\n";
        out << "path:    " << it->Path << "\n";
        out << "type:    " << ETypeToString(it->Type) << "\n";
        out << it->Text << "\n\n";
        elementNum++;
    }
}

void DumpBlocks(const vector<TContentBlock>& blocks, ostream& out) {
    for (size_t i = 0; i < blocks.size(); i++) {
        out << "block #" << i << "\n";
        out << "title: " << blocks[i].Title << "\n";
        out << "path:  " << blocks[i].Path << "\n";
        out << "text:  " << blocks[i].Text << "\n";
    }
}

TContentBlock::TContentBlock()
    : Type(BT_AdditionalContent)
    , Repeated(false)
{
}

} // NExhauster
