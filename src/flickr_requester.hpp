#include <cpprest/http_client.h>

#include <string>
#include <vector>


class flickr_requester {
private:
    const utility::string_t flickr_api_url = U("https://api.flickr.com/");
    const utility::string_t rest_api_url = U("/services/rest/");
    utility::string_t api_key;
    web::http::client::http_client flickr_client;

    utility::string_t form_request_url(const utility::string_t method_url, const std::unordered_map<utility::string_t, utility::string_t> args);
    pplx::task<web::json::value> extract_json(const web::http::method request_method, const utility::string_t request_url);
public:
    flickr_requester(const utility::string_t api_key);
    pplx::task<web::json::value> test_key();
    pplx::task<web::json::value> get_list(bool get_tags = false);
    pplx::task<web::json::value> get_info(const utility::string_t id);
    pplx::task<web::json::value> get_sizes(const utility::string_t id);
};