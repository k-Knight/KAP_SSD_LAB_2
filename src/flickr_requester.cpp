#include "flickr_requester.hpp"

#include <unordered_map>

#include <cpprest/filestream.h>

using namespace std;
using namespace pplx;
using namespace utility;

using namespace web;
using namespace web::http;
using namespace client;
using namespace concurrency::streams;

flickr_requester::flickr_requester(const string_t api_key)
    : api_key(api_key)
    , flickr_client(flickr_api_url)
{}

string_t flickr_requester::form_request_url(const string_t method_name, const unordered_map<string_t, string_t> args) {
    uri_builder uBld(this->rest_api_url);

    uBld.append_query("method", method_name);
    uBld.append_query("api_key", this->api_key);

    for( const auto& pair : args )
       uBld.append_query(pair.first, pair.second);

    uBld.append_query("format", "json");
    uBld.append_query("nojsoncallback", "1");

    return uBld.to_string();
}

task<json::value> flickr_requester::extract_json(const method request_method, const string_t request_url) {
    return this->flickr_client.request(request_method, request_url)
        .then([=] (http_response response) {
            cout << "[FLICKR REQUESTER]> got response with code: " << response.status_code() << "\n";
            return response.content_ready();
        }).then([=] (http_response response) {
            return response.extract_json();
        });
}

task<json::value> flickr_requester::get_list(bool get_tags) {
    unordered_map<string_t, string_t> request_args = {
        {"page", "1"}, {"per_page", "500"}
    };
    if (get_tags)
        request_args.insert({"extras", "tags"});
    string_t request_url = form_request_url("flickr.interestingness.getList", request_args);
    cout << "[FLICKR REQUESTER]> sending request: flickr.interestingness.getList\n";

    return extract_json(methods::GET, request_url);
}

task<json::value> flickr_requester::get_info(const string_t id) {
    unordered_map<string_t, string_t> request_args = {
        {"photo_id", id}
    };
    string_t request_url = form_request_url("flickr.photos.getInfo", request_args);
    cout << "[FLICKR REQUESTER]> sending request: flickr.photos.getInfo: id: " << id << "\n";

    return extract_json(methods::GET, request_url);
}

task<json::value> flickr_requester::get_sizes(const string_t id) {
    unordered_map<string_t, string_t> request_args = {
        {"photo_id", id}
    };
    string_t request_url = form_request_url("flickr.photos.getSizes", request_args);
    cout << "[FLICKR REQUESTER]> sending request: flickr.photos.getSizes: id: " << id << "\n";

    return extract_json(methods::GET, request_url);
}

task<json::value> flickr_requester::test_key() {
    unordered_map<string_t, string_t> request_args;
    string_t request_url = form_request_url("flickr.test.echo", request_args);
    cout << "[FLICKR REQUESTER]> sending request: flickr.test.echo\n";
    
    return extract_json(methods::GET, request_url);
}