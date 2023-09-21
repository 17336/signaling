#ifndef _UTIL_H_
#define _UTIL_H_

#include <sstream>
#include <vector>

#include "rapidjson/document.h"
#include "rapidjson/rapidjson.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "type.h"

void response(Type::connection_ptr con, const std::string &msg);

void response(Type::connection_ptr con, const std::string &msg,
                     const std::vector<std::string> &kv);

std::string getString(const rapidjson::Document &doc);

#endif  // _UTIL_H_