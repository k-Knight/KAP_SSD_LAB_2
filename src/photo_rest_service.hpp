#include <string>
#include <cpprest/http_listener.h>

#include "flickr_requester.hpp"

class photo_rest_service {
private:
    utility::string_t api_key;
    flickr_requester requester;

    void get_request_handler(web::http::http_request request);
    static web::json::value form_json_fail_message(const std::exception& e);
    static void send_json_response(web::http::http_request request, web::json::value json_value);
    static void send_html_response(web::http::http_request request, utility::string_t html_code);
    static pplx::task<web::json::value> catch_json_exception(pplx::task<web::json::value> json_task);
    static void process_tags(const utility::string_t& utf8_str, std::vector<utility::string_t>& tag_array);
    static utility::string_t get_best_image_resolution_url(web::json::value& img_sizes);
    static size_t get_integer_value(web::json::value& json_value);
    bool test_key();
public:
    photo_rest_service(const utility::string_t api_key);
    int run();
};