#include "../../include/json_util.hpp"
#include <map>
#include <string>
#include <cctype>
using namespace std;

string extract_value(const string &json, const string &key) {
    string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == string::npos) return "";
    size_t colon = json.find(':', p + pat.size());
    if (colon == string::npos) return "";
    size_t start = json.find_first_not_of(" \t\n\r", colon + 1);
    if (start == string::npos) return "";
    if (json[start] == '"') {
        size_t s2 = start + 1;
        size_t e2 = json.find('"', s2);
        if (e2 == string::npos) return "";
        return json.substr(s2, e2 - s2);
    } else {
        size_t end = json.find_first_of(",}", start);
        if (end == string::npos) end = json.size();
        string val = json.substr(start, end - start);
        size_t a = val.find_first_not_of(" \t\n\r");
        size_t b = val.find_last_not_of(" \t\n\r");
        if (a == string::npos) return "";
        return val.substr(a, b - a + 1);
    }
}

map<string,string> parse_json_simple(const string &json) {
    map<string,string> out;
    string cmd = extract_value(json, "cmd"); if (cmd!="") out["cmd"]=cmd;
    string u = extract_value(json, "username"); if (u!="") out["username"]=u;
    string p = extract_value(json, "password"); if (p!="") out["password"]=p;
    string rid = extract_value(json, "request_id"); if (rid!="") out["request_id"]=rid;
    string sid = extract_value(json, "session_id"); if (sid!="") out["session_id"]=sid;
    string op = extract_value(json, "operation"); if (op!="") out["operation"]=op;
    return out;
}