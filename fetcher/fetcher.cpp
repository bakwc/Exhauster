#include <sstream>
#include <curl/curl.h>
#include <boost/thread/detail/singleton.hpp>

#include "fetcher.h"

namespace NHttpFetcher {

size_t write_data(char *ptr, size_t size, size_t nmemb, void *userdata) {
    string* str = (string*)userdata;
    size_t count = size * nmemb;
    *str += std::string(ptr, count);
    return count;
}

class TCurl {
public:
    TCurl() {
        curl_global_init(CURL_GLOBAL_ALL);
    }
    ~TCurl() {
    }
    TResponse FetchUrl(const TRequest& request) {
        CURL* curl;
        ostringstream stream;
        TResponse response;

        curl = curl_easy_init();
        if (!curl) {
            //todo: throw exception here
            cerr << "failed to initialize curl\n";
            _exit(42);
        }
        curl_easy_setopt(curl, CURLOPT_URL, request.Url.c_str());
        if (request.User.is_initialized()) {
            string userPwd = *request.User + ":";
            if (request.Password.is_initialized()) {
                userPwd += *request.Password;
            }
            curl_easy_setopt(curl, CURLOPT_USERPWD, userPwd.c_str());
        }
        curl_easy_setopt(curl, CURLOPT_USERAGENT, request.UserAgent.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, request.Timeout.count() / 2);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, request.Timeout.count() / 2);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.Data);
        curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &response.Headers);
        CURLcode result = curl_easy_perform(curl); /* ignores error */
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.Code);

        char* resolvedUrl;
        curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &resolvedUrl);
        if (resolvedUrl) {
            response.ResolvedUrl = resolvedUrl;
        } else {
            response.ResolvedUrl = request.Url;
        }
        response.RequestUrl = request.Url;
        curl_easy_cleanup(curl);

        response.Success = true;

        if (result != CURLE_OK) {
            response.Success = false;
            response.Error = curl_easy_strerror(result);
        }

        return response;
    }
private:
} Curl;

TRequest::TRequest()
    : UserAgent("Mozilla/5.0 (compatible; Unknown-Fetcher)")
    , Timeout(chrono::seconds(10))
{
}

optional<string> FetchUrl(const string& url, chrono::milliseconds timeout) {
    TRequest request;
    request.Url = url;
    request.Timeout = timeout;
    TResponse response = FetchUrl(request);
    if (response.Success) {
        return response.Data;
    }
    return optional<string>();
}

TResponse FetchUrl(const TRequest& request) {
    return Curl.FetchUrl(request);
}

} // NHttpFetcher
