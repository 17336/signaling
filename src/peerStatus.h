#ifndef _PEERSTATUS_H_
#define _PEERSTATUS_H_

// peer status about room/session
class PeerStatus {
public:
    PeerStatus();
    PeerStatus(const PeerStatus&);
    PeerStatus& operator=(const PeerStatus&);

    void reset();

    bool isInSession() const;

    bool wasInSession() const;

    void setIsInSession(bool isInSession);

    bool isCameraUsing() const;

    void setCameraUsing(bool cameraUsing);

    bool isCameraUsed() const;

    bool isAudioUsing() const;

    void setAudioUsing(bool audioUsing);

    bool isAudioUsed() const;

    bool isScreenUsing() const;

    void setScreenUsing(bool screenUsing);

    bool isScreenUsed() const;

    bool isSendOffer() const;

    void setSendOffer(bool sendOffer);

    bool isReceiveOffer() const;

    void setReceiveOffer(bool receiveOffer);

    bool isSendAnswer() const;

    void setSendAnswer(bool sendAnswer);

    bool isReceiveAnswer() const;

    void setReceiveAnswer(bool receiveAnswer);

    bool isSendCandidate() const;

    void setSendCandidate(bool sendCandidate);

    bool isReceiveCandidate() const;

    void setReceiveCandidate(bool receiveCandidate);

    bool isConnected() const;

    void setConnected(bool connected);

private:
    bool isInSession_;
    bool wasInSession_;
    bool cameraUsing_;
    bool cameraUsed_;
    bool audioUsing_;
    bool audioUsed_;
    bool screenUsing_;
    bool screenUsed_;
    bool sendOffer_;
    bool receiveOffer_;
    bool sendAnswer_;
    bool receiveAnswer_;
    bool sendCandidate_;
    bool receiveCandidate_;
    bool connected_;
};


#endif // _PEERSTATUS_H_