#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <chrono>
#include <functional>

#include <curl/curl.h>
#include <utils/string.h>
#include <libexhaust/exhauster.h>
#include <server/server.h>
#include <fetcher/fetcher.h>

using namespace std;
using namespace placeholders;
using namespace NHttpServer;

class TServer {
public:
    TServer()
        : Server(NHttpServer::TSettings(8686))
    {
        Server.HandleAction("/exhause", std::bind(&TServer::ProcessExhauseRequest, this, _1));
    }
    optional<TResponse> ProcessExhauseRequest(const TRequest& request) {
        TResponse response;
        response.ContentType("text/plain");
        optional<string> url = request.GetParam("url");
        if (!url) {
            response.Data = "error: missing url";
            return response;
        }
        optional<string> data = NHttpFetcher::FetchUrl(*url);
        if (!data) {
            response.Data = "error: failed to fetch url";
            return response;
        }
        response.Data = NExhauster::ExhausteMainContent(*data).Text;
        return response;
    }
private:
    THttpServer Server;
};

int main(int argc, char** argv) {
    setlocale(LC_CTYPE, "en_US.UTF-8");

    TServer server;
    this_thread::sleep_for(chrono::system_clock::duration::max());

    return 0;
}
