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
        : Server(NHttpServer::TSettings(8687))
    {
        Server.HandleAction("/exhause", std::bind(&TServer::ProcessExhauseRequest, this, _1));
        Server.HandleAction("/exhause/post", std::bind(&TServer::ProcessExhausePostRequest, this, _1));
    }
    optional<TResponse> ProcessExhauseRequest(const TRequest& request) {
        TResponse response;
        response.ContentType("text/plain");
        optional<string> url = request.GetParam("url");
        if (!url) {
            response.Data = "error: missing url";
            return response;
        }
        optional<string> data = NHttpFetcher::FetchUrl(*url, chrono::seconds(30));
        if (!data) {
            response.Data = "error: failed to fetch url";
            return response;
        }
        NExhauster::TSettings settings;
        settings.DebugOutput = &cerr;
        response.Data = NExhauster::ExhausteMainContent(*data, settings).Text;
        return response;
    }
    optional<TResponse> ProcessExhausePostRequest(const TRequest& request) {
        TResponse response;
        response.ContentType("text/plain");
        if (request.Method != "POST") {
            response.Data = "error: no post data detected";
            return response;
        }
        response.Data = NExhauster::ExhausteMainContent(request.PostData).Text;
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
