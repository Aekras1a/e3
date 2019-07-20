#pragma once

#include <string>
using std::string;

struct Params
{
    string cmd;
    string cfg;
    string name;
    string userdir;
    bool userdirset = false;
    string password;
    bool passwordset = false;
    enum class Sign { UNDEFINED, SIGNED, UNSIGNED } sign = Sign::UNDEFINED;
    int bitsize = 0;
    string file;
    string cgtdir;
};

Params parse(int ac, char ** av);

std::string errorMsg(string s);

