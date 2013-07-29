#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <mutex>
#include <memory>
#include <chrono>
#include <functional>

#include <curl/curl.h>
#include <utils/string.h>
#include <libexhaust/exhauster.h>
#include <server/server.h>
#include <server/content_type.h>
#include <fetcher/fetcher.h>
#include <contrib/json/json.h>
#include <web/resources.h>
#include <contrib/cpptemplate/cpptempl.h>

using namespace std;
using namespace placeholders;
using namespace NHttpServer;

const size_t LIMIT_MINUTE = 30;
const size_t LIMIT_DAY = 2000;

enum EProcessError {
    PR_Ok,
    PR_MissingUrl,
    PR_FetchingFailed,
    PR_OutOfLimit
};

string ProcessErrorToString(EProcessError error) {
    switch (error) {
    case PR_Ok: return "everything ok";
    case PR_MissingUrl: return "missing url";
    case PR_FetchingFailed: return "fetching faild";
    case PR_OutOfLimit: return "out of limit";
    }
    return "";
}

struct TStats {
    TStats()
        : MinuteRequests(0)
        , DayRequests(0)
    {
    }
    size_t MinuteRequests;
    size_t DayRequests;
};

typedef chrono::time_point<std::chrono::system_clock> TTimePoint;

class TLimitManager {
public:
    TLimitManager(const string& stateFile,
                  size_t dayLimit,
                  size_t minuteLimit)
        : StateFile(stateFile)
        , DayLimit(dayLimit)
        , MinuteLimit(minuteLimit)
    {
        Load();
    }
    // true, if user is can to make a request
    bool Request(long ip) {
        lock_guard<mutex> guard(Lock);
        TStats& stats = Requests[ip];
        if (stats.DayRequests > LIMIT_DAY ||
                stats.MinuteRequests > LIMIT_MINUTE)
        {
            return false;
        }
        stats.DayRequests++;
        stats.MinuteRequests++;
        return true;
    }
    void Run() {
        while (true) {
            this_thread::sleep_for(chrono::minutes(1));
            Dump();
            lock_guard<mutex> guard(Lock);
            if (LastLimitsClear + chrono::hours(24) > chrono::system_clock::now()) {
                LastLimitsClear = chrono::system_clock::now();
                Requests.clear();
            }
        }
    }
    void Dump() {
        std::unordered_map<long, TStats> requests;
        {
            lock_guard<mutex> guard(Lock);
            requests = Requests;
        }

        Json::Value root;
        Json::StyledWriter writer;
        root["last_clear"] = (Json::UInt)chrono::duration_cast<chrono::seconds>(
                            LastLimitsClear.time_since_epoch()).count();
        for (auto& r: Requests) {
            root["requests"][boost::lexical_cast<string>(r.first)] = (Json::UInt)r.second.DayRequests;
        }
        string data = writer.write(root);
        try {
            SaveFile(StateFile, data);
        } catch (const std::exception& e) {
            cerr << "ERROR: failed to save requests stats. Reason: " << e.what() << "\n";
        }
    }
    void Load() {
        string data;
        try {
            data = LoadFile(StateFile);
        } catch (const std::exception& e) {
            cerr << "WARNING: failed to load requests stats. Starting with clear stats\n";
            return;
        }
        Json::Reader reader;
        Json::Value root;

        if (!reader.parse(data, root)) {
            cerr << "WARNING: failed to load requests stats. Starting with clear stats\n";
            return;
        }

        try {
            chrono::seconds lastLimitsClear(root["last_clear"].asUInt());
            std::unordered_map<long, TStats> newRequests;
            Json::Value requests = root["requests"];
            vector<string> ipAddresses = requests.getMemberNames();
            for (size_t i = 0; i < ipAddresses.size(); i++) {
                TStats stats;
                string ip = ipAddresses[i];
                stats.DayRequests = requests[ip].asUInt();
                newRequests[boost::lexical_cast<long>(ip)] = stats;
            }

            lock_guard<mutex> guard(Lock);
            LastLimitsClear = TTimePoint(lastLimitsClear);
            Requests = newRequests;
        } catch (const std::exception& e) {
            cerr << "ERROR: failed to parse requests stats. Reason: " << e.what() << "\n";
            return;
        }

    }

    inline size_t GetDayLimit() {
        return DayLimit;
    }

    inline size_t GetMinuteLimit() {
        return MinuteLimit;
    }
private:
    mutex Lock;
    std::unordered_map<long, TStats> Requests;
    string StateFile;
    TTimePoint LastLimitsClear;
    size_t DayLimit;
    size_t MinuteLimit;
};

class TServer {
public:
    TServer(const string& configFile)
        : Server()
    {
        Json::Reader reader;
        Json::Value root;
        string requestStateFile;
        string configData;

        configData = LoadFile(configFile);
        if (!reader.parse(configData, root)) {
            cerr << "ERROR: failed to parse config file '" << configFile << "'\n";
            _exit(42);
            // todo: throw exception here
        }

        requestStateFile = root["requests_state_file"].asString();
        ServerPort = root["server_port"].asUInt();
        Threads = root["threads"].asUInt();
        size_t dayLimit = root["day_limit"].asUInt();
        size_t minuteLimit = root["minute_limit"].asUInt();

        RequestsLimitManager.reset(new TLimitManager(requestStateFile, dayLimit, minuteLimit));
    }

    void Run() {
        NHttpServer::TSettings settings(ServerPort);
        settings.Threads = Threads;
        Server.reset(new NHttpServer::THttpServer(settings));
        Server->HandleAction("/exhause", std::bind(&TServer::ProcessExhauseRequest, this, _1));
        Server->HandleAction("/", std::bind(&TServer::ProcessMainRequest, this, _1));
        //Server.HandleAction("/exhause/post", std::bind(&TServer::ProcessExhausePostRequest, this, _1));
        Server->HandleActionDefault(std::bind(&TServer::ProcessStaticRequest, this, _1));
        RequestsLimitManager->Run();
    }

    optional<TResponse> ProcessMainRequest(const TRequest& request) {
        string data;
        bool result = GetResource("/index.html", data);
        assert(result && "index.html not found in resources");
        cpptempl::data_map values;
        values["day_limit"] = RequestsLimitManager->GetDayLimit();
        values["minute_limit"] = RequestsLimitManager->GetMinuteLimit();


        {
            size_t i = 0;
            string status = ProcessErrorToString((EProcessError)i);
            do {
                cpptempl::data_map statusMap;
                statusMap["status"] = status;
                statusMap["code"] = i;
                values["statuses"].push_back(statusMap);
                i++;
                status = ProcessErrorToString((EProcessError)i);
            } while (!status.empty());
        }

        cpptempl::parse(data, values);
        TResponse response;
        response.Code = 200;
        response.ContentType("text/html;charset=UTF-8");
        response.Data = cpptempl::parse(data, values);
        return response;
    }

    optional<TResponse> ProcessStaticRequest(const TRequest& request) {
        string staticResource;
        string resource = request.URI;
        if (GetResource(resource, staticResource)) {
            TResponse response;
            response.Data = staticResource;
            response.Code = 200;
            response.ContentType(ContentTypeByName(resource) + ";charset=UTF-8");
            return response;
        }
        return optional<TResponse>();
    }

    optional<TResponse> ProcessExhauseRequest(const TRequest& request) {
        TResponse response;
        Json::Value root;
        Json::StyledWriter writer;
        response.ContentType("text/plain;charset=utf-8");
        optional<string> url = request.GetParam("url");
        EProcessError status = PR_Ok;

        if (!RequestsLimitManager->Request(request.IP)) {
            status = PR_OutOfLimit;
        } else if (!url) {
            status = PR_MissingUrl;
        } else {
            NHttpFetcher::TRequest request;
            request.Url = *url;
            request.Timeout = chrono::seconds(30);
            NHttpFetcher::TResponse response = NHttpFetcher::FetchUrl(request);

            if (!response.Success) {
                status = PR_FetchingFailed;
            } else {
                NExhauster::TSettings settings;
                //settings.DebugOutput = &cerr;
                optional<string> charset = response.ParseCharset();
                if (charset.is_initialized()) {
                    settings.Charset = *charset;
                }
                NExhauster::TContentBlock result;
                result = NExhauster::ExhausteMainContent(response.Data, settings);
                root["title"] = result.Title;
                root["text"] = result.Text;
            }
        }
        root["status"] = status;
        if (status != PR_Ok) {
            root["error"] = ProcessErrorToString(status);
        }

        response.Data = writer.write(root);
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
    unsigned short ServerPort;
    size_t Threads;
    unique_ptr<THttpServer> Server;
    unique_ptr<TLimitManager> RequestsLimitManager;
};

int main(int argc, char** argv) {
    setlocale(LC_CTYPE, "en_US.UTF-8");

    if (argc != 2) {
        cerr << "\n  Usage " + string(argv[0]) + " config.json\n\n";
        return 42;
    }

    TServer server(argv[1]);
    server.Run();

    return 0;
}
