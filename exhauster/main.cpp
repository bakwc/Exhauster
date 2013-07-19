#include <iostream>
#include <string>

#include <utils/string.h>

#include <libexhaust/exhauster.h>

using namespace std;

int main() {
    setlocale(LC_CTYPE, "en_US.UTF-8");

    string data = "<html><body>Some regular content is here. Text text text.</body></html>";
    cout << NExhauster::ExhausteMainContent(data).Text << "\n";
    return 0;
}
