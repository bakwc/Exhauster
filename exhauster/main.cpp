#include <iostream>
#include <string>

#include <utils/string.h>
#include <fstream>

#include <libexhaust/exhauster.h>

using namespace std;

int main(int argc, char** argv) {
    if (argc < 2) {
        return 42;
    }

    setlocale(LC_CTYPE, "en_US.UTF-8");

    ifstream in(argv[1]);
    string data((istreambuf_iterator<char>(in)), (istreambuf_iterator<char>()));

    //string data = "<html><body>Some regular content is here. Text text text.</body></html>";

    NExhauster::TSettings settings;
    //settings.DebugOutput = &cout;

    cout << NExhauster::ExhausteMainContent(data, settings).Text << "\n";
    return 0;
}
