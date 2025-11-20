#include "../../include/json_util.hpp"
#include <string>
#include <map>
#include <cctype>

using namespace std;

map<string,string> parse_json_simple(const string &json) {
    map<string,string> out;
    size_t i = 0, n = json.size();

    auto skip = [&](void){
        while (i < n && isspace((unsigned char)json[i])) ++i;
    };

    while (i < n && json[i] != '{') ++i;
    if (i == n) return out;
    ++i;

    while (i < n) {
        skip();
        if (i < n && json[i] == '}') break;

        if (json[i] == '\"') {
            ++i;
            string key;
            while (i < n && json[i] != '\"') { key.push_back(json[i++]); }
            if (i < n && json[i] == '\"') ++i;
            skip();
            if (i < n && json[i] == ':') ++i;
            skip();

            string val;
            if (i < n && json[i] == '\"') {
                ++i;
                while (i < n && json[i] != '\"') { val.push_back(json[i++]); }
                if (i < n && json[i] == '\"') ++i;
            } else {
                while (i < n && json[i] != ',' && json[i] != '}') {
                    val.push_back(json[i++]);
                }
                size_t s = 0;
                while (s < val.size() && isspace((unsigned char)val[s])) ++s;
                size_t e = val.size();
                while (e > s && isspace((unsigned char)val[e-1])) --e;
                val = val.substr(s, e - s);
                if (val.size() >= 2 && val.front() == '"' && val.back() == '"') {
                    val = val.substr(1, val.size()-2);
                }
            }
            out[key] = val;
        } else {
            ++i;
        }

        while (i < n && json[i] != ',' && json[i] != '}') ++i;
        if (i < n && json[i] == ',') ++i;
    }

    return out;
}