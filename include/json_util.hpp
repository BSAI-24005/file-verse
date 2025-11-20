#ifndef JSON_UTIL_HPP
#define JSON_UTIL_HPP

#include <string>
#include <map>

std::map<std::string,std::string> parse_json_simple(const std::string &json);

#endif