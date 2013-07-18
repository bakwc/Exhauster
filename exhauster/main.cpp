#include <iostream>
#include <string>

#include <utils/string.h>

using namespace std;

int main() {
    string str = "проверка\n";
    wstring wtr = UTF8ToWide(str);
    string str2 = WideToUTF8(wtr);
    cout << str2;
    return 0;
}