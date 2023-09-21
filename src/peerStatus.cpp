#include "peerStatus.h"

PeerStatus::PeerStatus()
    : wasInSession_(false),
      isInSession_(false),
      cameraUsing_(false),
      cameraUsed_(false),
      audioUsing_(false),
      audioUsed_(false),
      screenUsing_(false),
      screenUsed_(false),
      sendOffer_(false),
      receiveOffer_(false),
      sendAnswer_(false),
      receiveAnswer_(false),
      sendCandidate_(false),
      receiveCandidate_(false),
      connected_(false) {}

bool PeerStatus::isInSession() const { return isInSession_; }

bool PeerStatus::wasInSession() const { return wasInSession_; }

void PeerStatus::setIsInSession(bool isInSession) {
    isInSession_ = isInSession;
    if (isInSession)
        wasInSession_ = true;
}

bool PeerStatus::isCameraUsing() const { return cameraUsing_; }

void PeerStatus::setCameraUsing(bool cameraUsing) {
    cameraUsing_ = cameraUsing;
    if (cameraUsing)
        cameraUsed_ = true;
}

bool PeerStatus::isCameraUsed() const { return cameraUsed_; }

bool PeerStatus::isAudioUsing() const { return audioUsing_; }

void PeerStatus::setAudioUsing(bool audioUsing) {
    audioUsing_ = audioUsing;
    if (audioUsing)
        audioUsed_ = true;
}

bool PeerStatus::isAudioUsed() const { return audioUsed_; }

bool PeerStatus::isScreenUsing() const { return screenUsing_; }

void PeerStatus::setScreenUsing(bool screenUsing) {
    screenUsing_ = screenUsing;
    if (screenUsing)
        screenUsed_ = true;
}

bool PeerStatus::isScreenUsed() const { return screenUsed_; }

bool PeerStatus::isSendOffer() const { return sendOffer_; }

void PeerStatus::setSendOffer(bool sendOffer) { sendOffer_ = sendOffer; }

bool PeerStatus::isReceiveOffer() const { return receiveOffer_; }

void PeerStatus::setReceiveOffer(bool receiveOffer) {
    receiveOffer_ = receiveOffer;
}

bool PeerStatus::isSendAnswer() const { return sendAnswer_; }

void PeerStatus::setSendAnswer(bool sendAnswer) { sendAnswer_ = sendAnswer; }

bool PeerStatus::isReceiveAnswer() const { return receiveAnswer_; }

void PeerStatus::setReceiveAnswer(bool receiveAnswer) {
    receiveAnswer_ = receiveAnswer;
}

bool PeerStatus::isSendCandidate() const { return sendCandidate_; }

void PeerStatus::setSendCandidate(bool sendCandidate) {
    sendCandidate_ = sendCandidate;
}

bool PeerStatus::isReceiveCandidate() const { return receiveCandidate_; }

void PeerStatus::setReceiveCandidate(bool receiveCandidate) {
    receiveCandidate_ = receiveCandidate;
}

bool PeerStatus::isConnected() const { return connected_; }

void PeerStatus::setConnected(bool connected) { connected_ = connected; }

void PeerStatus::reset() {
    wasInSession_ = false;
    isInSession_ = false;
    cameraUsing_ = false;
    cameraUsed_ = false;
    audioUsing_ = false;
    audioUsed_ = false;
    screenUsing_ = false;
    screenUsed_ = false;
    sendOffer_ = false;
    receiveOffer_ = false;
    sendAnswer_ = false;
    receiveAnswer_ = false;
    sendCandidate_ = false;
    receiveCandidate_ = false;
    connected_ = false;
}

PeerStatus::PeerStatus(const PeerStatus &other) {
    wasInSession_ = other.wasInSession_;
    isInSession_ = other.isInSession_;
    cameraUsing_ = other.cameraUsing_;
    cameraUsed_ = other.cameraUsed_;
    audioUsing_ = other.audioUsing_;
    audioUsed_ = other.audioUsed_;
    screenUsing_ = other.screenUsing_;
    screenUsed_ = other.screenUsed_;
    sendOffer_ = other.sendOffer_;
    receiveOffer_ = other.receiveOffer_;
    sendAnswer_ = other.sendAnswer_;
    receiveAnswer_ = other.receiveAnswer_;
    sendCandidate_ = other.sendCandidate_;
    receiveCandidate_ = other.receiveCandidate_;
    connected_ = other.connected_;
}

PeerStatus &PeerStatus::operator=(const PeerStatus &other) {
    wasInSession_ = other.wasInSession_;
    isInSession_ = other.isInSession_;
    cameraUsing_ = other.cameraUsing_;
    cameraUsed_ = other.cameraUsed_;
    audioUsing_ = other.audioUsing_;
    audioUsed_ = other.audioUsed_;
    screenUsing_ = other.screenUsing_;
    screenUsed_ = other.screenUsed_;
    sendOffer_ = other.sendOffer_;
    receiveOffer_ = other.receiveOffer_;
    sendAnswer_ = other.sendAnswer_;
    receiveAnswer_ = other.receiveAnswer_;
    sendCandidate_ = other.sendCandidate_;
    receiveCandidate_ = other.receiveCandidate_;
    connected_ = other.connected_;
    return *this;
}