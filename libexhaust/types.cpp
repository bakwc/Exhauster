#include "types.h"

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
