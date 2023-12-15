#include "image_detect.hpp"
#include "misc.hpp"
#include <cppcodec/base64_rfc4648.hpp>
#include <cstdlib>
#include <curl/curl.h>
using base64 = cppcodec::base64_rfc4648;

std::string img_to_base64(cv::Mat image) {
    std::vector<uchar> buf;
    cv::imencode(".jpg", image, buf);
    auto base64_str = base64::encode(buf.data(), buf.size());
    return base64_str;
}

cv::Mat base64_to_img(const std::string& base64_str) {
    auto decoded_data = base64::decode(base64_str.c_str(), base64_str.size());
    cv::Mat decoded_image = cv::imdecode(decoded_data, cv::IMREAD_COLOR);
    return decoded_image;
}

struct memory {
    char* response;
    size_t size;
};

static size_t cb(void* data, size_t size, size_t nmemb, void* clientp) {
    size_t realsize = size * nmemb;
    struct memory* mem = (struct memory*)clientp;

    char* ptr = (char*)realloc(mem->response, mem->size + realsize + 1);
    if (ptr == nullptr)
        return 0; // out of memory!

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), data, realsize);
    mem->size += realsize;
    mem->response[mem->size] = 0;

    return realsize;
}

nlohmann::json detect_image(nlohmann::json detect_data) {
    std::string json_str = detect_data.dump();
    struct memory chunk = {0};
    CURL* curl = curl_easy_init();
    struct curl_slist* list = nullptr;
    if (curl != nullptr) {
        char buf[CURL_ERROR_SIZE];
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, buf);
#define XX(xx)                    \
    if (CURLE_OK != (xx)) {       \
        THROW_RUNTIME_ERROR(buf); \
    }
        XX(curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.2.179:8081/algorithms_management_server/config_temp_rec/sfyl"));
        // XX(curl_easy_setopt(curl, CURLOPT_URL, "http://httpbin.org/post"));
        XX(curl_easy_setopt(curl, CURLOPT_POST, 1L));
        list = curl_slist_append(list, "Content-Type: application/json");
        list = curl_slist_append(list, "Accept: application/json");
        XX(curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list));
        XX(curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str()));
        XX(curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb));
        XX(curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk));
        XX(curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 12L));
        XX(curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L));
        XX(curl_easy_perform(curl));
        curl_easy_cleanup(curl);
#undef XX
    }
    curl_slist_free_all(list); /* free the list */
    nlohmann::json result = nlohmann::json::parse(chunk.response);
    free(chunk.response); // Don't forget to free memory after using
    return result;
}
