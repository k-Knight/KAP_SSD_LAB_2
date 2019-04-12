#include "photo_rest_service.hpp"

#include <iostream>
#include <vector>
#include <thread>
#include <mutex> 

using namespace std;
using namespace utility;
using namespace placeholders;
using namespace this_thread;
using namespace chrono;

using namespace web;
using namespace web::http;
using namespace http::experimental::listener;
using namespace pplx;

photo_rest_service::photo_rest_service(const string_t api_key)
    : requester(api_key)
    , api_key(api_key)
{}

int photo_rest_service::run() {
    cout << "[PHOTO REST SERVICE]> Service initialization started\n";

    http_listener listener("http://0.0.0.0:80/interestingPhoto/");

    listener.support(methods::GET, bind(&photo_rest_service::get_request_handler, this, _1));
    try {
        listener.open().then([&listener]() {
            cout << "[PHOTO REST SERVICE]> Service is listening to " << listener.uri().to_string() << "\n";
        }).wait();
        if (!test_key())
            throw runtime_error("API key is not valid");
        else
            cout << "[PHOTO REST SERVICE]> API key is valid\n";

        cout << "[PHOTO REST SERVICE]> initialization complete\n";
        sleep_until(time_point<system_clock>::max());

        return 0;
    } catch (const exception& e) {
        cout << "[ERROR]> " << e.what() << '\n';
        cout << "[PHOTO REST SERVICE]> initialization error\n";
        return 1;
    }
}

bool photo_rest_service::test_key() {
    return catch_json_exception(requester.test_key())
        .then([=] (json::value json_value) {
            try{
                if (json_value["stat"].as_string() == "ok")
                    return true;
            } catch (const exception& e) { }

            return false;
        }).get();
}

json::value photo_rest_service::form_json_fail_message(const exception& e) {
    stringstream stream;
    stream << "{\"status\": \"fail\", \"error\": \"" << e.what() << "\"}";
    return json::value::parse(stream.str());
}

task<json::value> photo_rest_service::catch_json_exception(task<json::value> json_task) {
    return json_task.then([] (task<json::value> prev){
            json::value retVal;
            try {
                return prev.get();
            } catch (const exception& e) {
                cout << "[REQUEST ERROR]> " << e.what() << "\n";
                return form_json_fail_message(e);
            }
        });
}

void photo_rest_service::send_json_response(http_request request, json::value json_value) {
    http_response response;
    response.headers().add("Content-type", "text/json");
    try {
        response.set_status_code(status_codes::OK);
        response.set_body(json_value.serialize());
    } catch (const exception& e) {
        cout << "[REPLY ERROR]> " << e.what() << "\n";
        response.set_status_code(status_codes::BadRequest);
        response.set_body(form_json_fail_message(e));
    }
    request.reply(response);
}

void photo_rest_service::send_html_response(http_request request, string_t html_code) {
    http_response response;
    response.headers().add("Content-type", "text/html");
    response.headers().add("Cache-Control", "no-cache");
    response.set_status_code(status_codes::OK);
    response.set_body(html_code);

    request.reply(response);
}

void photo_rest_service::process_tags(const string_t& utf8_str, vector<string_t>& tag_array) { 
    stringstream_t tags_stream(utf8_str);
    string_t tag;
    locale utf8_locale("en_US.UTF-8");
    while (getline(tags_stream, tag, ' ')) {
        locale loc("en_US.utf8");
        u16string uTag = conversions::utf8_to_utf16(tag);
        wstring wTag(uTag.begin(), uTag.end());
    
        auto& f = use_facet<ctype<wchar_t>>(loc);
        f.tolower(&wTag[0], &wTag[wTag.size()]);

        wstring wTagCut;
        for(size_t i = 0; i < wTag.size(); i++)
            if (isalnum(wTag[i], loc)) wTagCut += tolower(wTag[i], loc);

        uTag = u16string(wTagCut.begin(), wTagCut.end());

        tag = conversions::utf16_to_utf8(uTag);
        if (find(tag_array.begin(), tag_array.end(), tag) == tag_array.end())
            tag_array.emplace_back(tag);
    }
}

size_t photo_rest_service::get_integer_value(json::value& json_value) {
    if (json_value.is_integer())
        return json_value.as_integer();
    else
        return stoull(conversions::to_utf8string(json_value.as_string()));
}

string_t photo_rest_service::get_best_image_resolution_url(json::value& img_sizes) {
    string_t result = "";
    size_t largest_area = 0;

    for (auto& json_size : img_sizes.as_array()) {
        size_t img_area = get_integer_value(json_size["width"]) * get_integer_value(json_size["height"]);

        if (json_size["label"].as_string() == "Original")
            return json_size["source"].as_string();
        else if (img_area > largest_area) {
            largest_area = img_area;
            result = json_size["source"].as_string();
        }
    }

    return result;
}

void photo_rest_service::get_request_handler(http_request request) {    
    stringstream_t request_path(conversions::to_utf8string(uri::decode(request.relative_uri().to_string())));
    string_t element;
    vector<string_t> path_elements;

    while(getline(request_path, element, '/'))
        if (element.size() > 0)
            path_elements.push_back(element);

    if (path_elements.size() > 0) {
        cout << "[PHOTO REST SERVICE]> finding photo with tag: " << path_elements[0] << "\n";
        vector<string_t> request_tag;
        process_tags(path_elements[0], request_tag);

        catch_json_exception(requester.get_list(true))
            .then([=] (json::value json_value) {
                vector<string_t> img_tags;
                try {
                    json_value = json_value["photos"]["photo"];
                    for (auto& photo_info : json_value.as_array()) {
                        string_t raw_tags = photo_info["tags"].as_string();
                        process_tags(raw_tags, img_tags);
                        for (const auto& tag : img_tags)
                            if (tag == request_tag[0]) {
                                return photo_info["id"].as_string();
                            }
                    }
                } catch (const exception& e) {
                    cout << "[PARSING ERROR]> " << e.what() << "\n";
                }

                return string_t("");
            }).then([=] (const string_t id) {
                if (id.size() > 0) {
                    catch_json_exception(requester.get_sizes(id))
                        .then([=] (json::value json_value){
                            try {
                                json_value = json_value["sizes"]["size"];
                                string_t img_url = get_best_image_resolution_url(json_value);

                                http_response response;

                                response.set_status_code(status_codes::MovedPermanently);
                                response.headers().add("Location", img_url);

                                request.reply(response);
                                return;
                            } catch (const exception& e) {
                                cout << "[PARSING ERROR]> " << e.what() << "\n";
                            }
                        });
                    return;
                }

                send_html_response(request, "<html><head><meta charset='UTF-8'><style>html, body {padding: 0;margin: 0;}body {padding: 0;margin: 0;width: 100%;background-color: #e0e0e0;font-family: -apple-system,BlinkMacSystemFont,Segoe UI,Helvetica,Arial,sans-serif,Apple Color Emoji,Segoe UI Emoji,Segoe UI Symbol;;}.center {min-height: calc(100vh - 200pt);width: 50%;background-color: white;margin: auto;margin-top: 99pt;margin-bottom: 99pt;border-radius:5pt;box-shadow: 0px 0px 10px #888888;}.full_w {padding: 0;margin: 0;width: 100%;min-height: 1px;max-height: 1px;}h1 {margin-top: 30pt;color: #2070a0;}.full_align {width: 100%;text-align: center;}.container {margin: auto;font-size: 14pt;}.text-container {width: 80%;margin-top: 30pt;text-align: justify;}.list-container {width: 90%;margin-top: 30pt;}ul {width: 100%;-webkit-column-count: 2;-moz-column-count: 2;column-count: 2;list-style-type: none;margin: 0;padding: 0;}li {padding: 5pt 18pt 5pt 18pt;margin: 0 0 10pt 0;box-shadow: 0px 0px 5px #bbbbbb inset;background-color: #eae8ff;border-radius: 2pt;}li > * {margin: 0;}</style></head><body><div class='center'><div class='full_w'></div><div class='full_align'><h1>Unable to find</h1></div><div class='full_w'></div><div class='container text-container'><p>There is no photo containing specified tag. You can navigate to <a href='http://localhost/interestingPhoto/'>http://localhost/interestingPhoto/</a> to see available tags</p></div></div></body></html>");
            });
    } else {
        catch_json_exception(requester.get_list(true))
            .then([=] (json::value json_value) {
                vector<string_t> tags;

                try {
                    json_value = json_value["photos"]["photo"];
                    for (auto& photo_info : json_value.as_array()) {
                        string_t raw_tags = photo_info["tags"].as_string();
                        process_tags(raw_tags, tags);
                    }
                } catch (const exception& e) {
                    cout << "[PARSING ERROR]> " << e.what() << "\n";
                }

                json_value = json::value();
                for (size_t i = 0; i < tags.size(); i++)
                    json_value["tags"][i] = json::value::string(tags[i]);

                return json_value;
            }).then([=] (json::value json_value) {
                string_t html_part = "<html><head><meta charset='UTF-8'><style>html, body {padding: 0;margin: 0;color: #303030;}body {padding: 0;margin: 0;width: 100%;background-color: #e0e0e0;font-family: -apple-system,BlinkMacSystemFont,Segoe UI,Helvetica,Arial,sans-serif,Apple Color Emoji,Segoe UI Emoji,Segoe UI Symbol;;}.center {min-height: calc(100vh - 80pt);width: 50%;background-color: white;margin: auto;margin-top: 39pt;margin-bottom: 39pt;border-radius:8pt;box-shadow: 0px 0px 10px #888888;}.full_w {padding: 0;margin: 0;width: 100%;min-height: 1px;max-height: 1px;}h1 {margin-top: 30pt;color: #2070a0;}.full_align {width: 100%;text-align: center;}.container {margin: auto;font-size: 14pt;}.text-container {width: 80%;margin-top: 30pt;text-align: justify;}.list-container {width: 90%;margin-top: 30pt;}ul {width: 100%;-webkit-column-count: 3;-moz-column-count: 3;column-count: 3;list-style-type: none;margin: 0;padding: 0;list-style-position: inside;margin-bottom: 30pt;}ul li{padding: 5pt 18pt 5pt 18pt;margin: 0 0 10pt 0;box-shadow: 0px 0px 5px #bbbbbb inset;background-color: #eae8ff;border-radius: 5pt;font-size: 10pt;}ul li > * {margin: 0;}a {text-decoration: none;color: #006080;font-family: Consolas NF, Consolas, Monospace; font-weight: bold;}</style></head><body><div class='center'><div class='full_w'></div><div class='full_align'><h1>Welcome to KAP_SSD_LAB_2</h1></div><div class='full_w'></div><div class='container text-container'><p>This is a placeholder page that lists all available tags. You can see them in a list below, click them to automatically make corresponding request.</p></div><div class='container list-container'><ul>";
                for (auto& tag : json_value["tags"].as_array()) {
                    html_part += "<li><a href='http://localhost/interestingPhoto/";
                    html_part += tag.as_string();
                    html_part += "/'>";
                    string_t display_text = tag.as_string();
                    if (display_text.size() > 21 ) {
                        display_text = display_text.substr(0, 18);
                        display_text += "...";
                    }
                    html_part += display_text;
                    html_part += "</a></li>";
                }
                html_part += "</ul></div><div class='full_w'></div></div></body></html>";

                send_html_response(request, html_part);
            });
    }
    return;
}