#ifndef LNK
#define LNK

#include "packet_m.h"
#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

class Lnk : public cSimpleModule {
private:
    cQueue buffer;
    cMessage* endServiceEvent = NULL;
    cOutVector bufferSizeVector;

public:
    Lnk();
    ~Lnk() override;

protected:
    void initialize() override;
    void finish() override;
    void handleMessage(cMessage* msg) override;
    void handleEndServiceEvent();
};

Define_Module(Lnk);

#endif /* LNK */

Lnk::Lnk() {
}

Lnk::~Lnk() {
    cancelAndDelete(endServiceEvent);
}

void Lnk::initialize() {
    endServiceEvent = new cMessage("endService");
    bufferSizeVector.setName("BufferSize");
}

void Lnk::finish() {
}

void Lnk::handleMessage(cMessage* msg) {
    if (msg == endServiceEvent) {
        handleEndServiceEvent();
    } else {
        if (msg->arrivedOn("toNet$i")) {
            // enqueue
            buffer.insert(msg);
            bufferSizeVector.record(buffer.getLength());
            // if the server is idle
            if (!endServiceEvent->isScheduled()) {
                // start the service now
                scheduleAt(simTime() + 0, endServiceEvent);
            }
        } else {
            // msg is from out, send to net
            send(msg, "toNet$o");
        }
    }
}

void Lnk::handleEndServiceEvent() {
    if (buffer.isEmpty()) {
        return;
    }

    cMessage* msg = dynamic_cast<cMessage*>(buffer.pop());
    bufferSizeVector.record(buffer.getLength());

    send(msg, "toOut$o");

    simtime_t serviceTime;
    if (cPacket* pkt = dynamic_cast<cPacket*>(msg)) {
        serviceTime = pkt->getDuration();
    } else {
        serviceTime = 0;
    }

    scheduleAt(simTime() + serviceTime, endServiceEvent);
}
