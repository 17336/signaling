#include "util.h"

void response(Type::connection_ptr con, const std::string &msg) {
    con->send("{\"msg\":\"" + msg + "\"}");
}


void response(Type::connection_ptr con, const std::string &msg,
                     const std::vector<std::string> &kv) {
    std::stringstream res;
    res << "{\"msg\":\"" << msg << "\",\"body\":{";
    int len = kv.size();
    for (int i = 1; i < len; i += 2) {
        bool isNumber = kv[i - 1].find("id") > 0;
        res << "\"" << kv[i - 1] << "\":" << (isNumber ? "\"" : "") << kv[i]
            << (isNumber ? "\"" : "");
        if (i + 2 < len)
            res << ",";
    }
    res << "}";
    con->send(res.str());
}

std::string getString(const rapidjson::Document &doc) {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}