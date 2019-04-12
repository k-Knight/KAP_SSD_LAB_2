#include <cpprest/http_client.h>
#include <iostream>
#include <string>

#include "photo_rest_service.hpp"

using namespace std;
using namespace utility;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cout << "An application key must be provided as an argument";
        return -1;
    }

    string_t api_key = argv[1];

    return photo_rest_service(api_key).run();
}