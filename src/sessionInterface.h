#ifndef _SESSIONINTERFACE_H_
#define _SESSIONINTERFACE_H_

#include <string>

class SessionNegotiate {
public:
    virtual bool sendSDPOffer(int64_t from_pid, int64_t dest_pid,
                              const std::string& offer) = 0;

    virtual bool sendSDPAnswer(int64_t from_pid, int64_t dest_pid,
                               const std::string& answer) = 0;
    virtual bool sendICECandidate(int64_t from_pid, int64_t dest_pid,
                                  const std::string& candidate) = 0;
    virtual bool connected(int64_t from_pid) = 0;
};

class SessionControl {
public:
    // 会话中控制
    virtual bool openCamera(int64_t from_pid) = 0;
    virtual bool closeCamera(int64_t from_pid) = 0;

    virtual bool openScreen(int64_t from_pid) = 0;
    virtual bool closeScreen(int64_t from_pid) = 0;

    virtual bool openAudio(int64_t from_pid) = 0;
    virtual bool closeAudio(int64_t from_pid) = 0;
};

#endif  // _SESSIONINTERFACE_H_