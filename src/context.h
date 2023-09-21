#ifndef _CONTEXT_H_
#define _CONTEXT_H_

#include "type.h"

class Context
{
public:
    Context() {}

    Context(Type::connection_ptr con, Type::message_ptr msg)
        : con_(con), msg_(msg) {}
    
    Context(const Context &other) {
        con_ = other.con_;
        msg_ = other.msg_;
    }

    void operator=(const Context &other) {
        con_ = other.con_;
        msg_ = other.msg_;
    }

    Type::connection_ptr con_;
    Type::message_ptr msg_;
};

#endif // _CONTEXT_H_