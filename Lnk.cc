#ifndef LNK
#define LNK

#include <omnetpp.h>
#include <packet_m.h>
#include <string.h>

using namespace omnetpp;

class Lnk : public cSimpleModule {
private:
    cQueue buffer;
    cMessage* endServiceEvent = NULL;
    simtime_t serviceTime;
    cOutVector bufferSizeVector;

public:
    Lnk();
    ~Lnk() override;

protected:
    void initialize() override;
    void finish() override;
    void handleMessage(cMessage* msg) override;
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
    bufferSizeVector.setName("Buffer Size");
}

void Lnk::finish() {
}

void Lnk::handleMessage(cMessage* msg) {
    if (msg == endServiceEvent) {
        if (!buffer.isEmpty()) {
            // dequeue
            Packet* pkt = dynamic_cast<Packet*>(buffer.pop());
            bufferSizeVector.record(buffer.getLength());
            // send
            send(pkt, "toOut$o");
            serviceTime = pkt->getDuration();
            scheduleAt(simTime() + serviceTime, endServiceEvent);
        }
    } else { // msg is a packet
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
