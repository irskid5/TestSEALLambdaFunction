// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#include "base64.h"
#include <iostream>
#include <stdexcept>

static const char binaryToBase64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const int base64ToBinaryTable[] = {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1,  0,  1,  2,  3,  4,  5,  6,
        7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        49, 50, 51
};

std::string base64_encode(const uint8_t *data, size_t length)
{
    std::string output;
    output.reserve(length * 5 / 3);
    int i;
    for(i = 0; i < length - 3; i += 3)
    {
        int value1 = data[i] >> 2;
        output.push_back(binaryToBase64Table[value1]);
        int value2 = ((data[i] & 3) << 4) | (data[i + 1] >> 4);
        output.push_back(binaryToBase64Table[value2]);
        int value3 = ((data[i + 1] & 15) << 2) | (data[i + 2] >> 6);
        output.push_back(binaryToBase64Table[value3]);
        int value4 = data[i + 2] & 63;
        output.push_back(binaryToBase64Table[value4]);
    }
    if(length - i == 1)
    {
        int value1 = data[i] >> 2;
        output.push_back(binaryToBase64Table[value1]);
        int value2 = (data[i] & 3) << 4;
        output.push_back(binaryToBase64Table[value2]);
        output.push_back('=');
        output.push_back('=');
    }
    else if(length - i == 2)
    {
        int value1 = data[i] >> 2;
        output.push_back(binaryToBase64Table[value1]);
        int value2 = ((data[i] & 3) << 4) | (data[i + 1] >> 4);
        output.push_back(binaryToBase64Table[value2]);
        int value3 = (data[i + 1] & 15) << 2;
        output.push_back(binaryToBase64Table[value3]);
        output.push_back('=');
    }
    else if(length - i == 3)

    {
        int value1 = data[i] >> 2;
        output.push_back(binaryToBase64Table[value1]);
        int value2 = ((data[i] & 3) << 4) | (data[i + 1] >> 4);
        output.push_back(binaryToBase64Table[value2]);
        int value3 = ((data[i + 1] & 15) << 2) | (data[i + 2] >> 6);
        output.push_back(binaryToBase64Table[value3]);
        int value4 = data[i + 2] & 63;
        output.push_back(binaryToBase64Table[value4]);
    }
    return output;
}

std::string base64_encode(const std::string &data)
{
    return base64_encode(reinterpret_cast<const uint8_t*>(data.c_str()), data.size());
}

static int encodedToIndex(int encoded)
{
    return base64ToBinaryTable[encoded];
}

std::string base64_decode(const std::string &encoded)
{
    std::string output;
    output.reserve(encoded.length());
    int i;
    for(i = 0; i < encoded.size() - 4; i += 4)
    {
        int value1 = (encodedToIndex(encoded[i]) << 2) | (encodedToIndex(encoded[i + 1]) >> 4);
        output.push_back(value1);
        int value2 = (encodedToIndex(encoded[i + 1]) << 4) | (encodedToIndex(encoded[i + 2]) >> 2);
        output.push_back(value2);
        int value3 = (encodedToIndex(encoded[i + 2]) << 6) | (encodedToIndex(encoded[i + 3]));
        output.push_back(value3);
    }
    int value1 = (encodedToIndex(encoded[i]) << 2) | (encodedToIndex(encoded[i + 1]) >> 4);
    output.push_back(value1);
    if(encoded[i + 2] != '=')
    {
        int value2 = (encodedToIndex(encoded[i + 1]) << 4) | (encodedToIndex(encoded[i + 2]) >> 2);
        output.push_back(value2);
    }
    if(encoded[i + 3] != '=')
    {
        int value3 = (encodedToIndex(encoded[i + 2]) << 6) | (encodedToIndex(encoded[i + 3]));
        output.push_back(value3);
    }
    return output;
}






