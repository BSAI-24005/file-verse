#ifndef JSON_UTIL_HPP
#define JSON_UTIL_HPP

#include <string>
#include <map>
using namespace std;

map<string,string> parse_json_simple(const string &json);
string extract_value(const string &json, const string &key);

#endif