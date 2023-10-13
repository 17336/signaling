#include "util.h"

#include <ctime>

void response(Type::connection_ptr con, const std::string &msg) {
    con->send("{\"msg\":\"" + msg + "\"}");
}

void response(Type::connection_ptr con, const std::string &msg,
              const std::vector<std::string> &kv) {
    std::stringstream res;
    res << "{\"msg\":\"" << msg << "\"";
    int len = kv.size();
    for (int i = 1; i < len; i += 2) {
        bool isNumber = (kv[i - 1].find("id") != std::string::npos);
        res << ",\"" << kv[i - 1] << "\":" << (isNumber ? "" : "\"") << kv[i]
            << (isNumber ? "" : "\"");
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

std::string nowTime() {
    std::time_t currentTime = std::time(nullptr);
    struct tm *tmTime = std::localtime(&currentTime);

    // 格式化为 DATETIME 字符串
    char datetimeBuffer[20];
    std::strftime(datetimeBuffer, sizeof(datetimeBuffer), "%Y-%m-%d %H:%M:%S",
                  tmTime);
    return std::string(datetimeBuffer);
}
