#pragma once

#include <string>

std::string base64_encode(const uint8_t *data, size_t length);
std::string base64_encode(const std::string &data);
std::string base64_decode(const std::string &encoded);



