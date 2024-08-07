#ifndef APP
#define APP

#include "packet_m.h"
#include <omnetpp.h>
#include <string.h>

using namespace omnetpp;

class App : public cSimpleModule {
private:
    cMessage* sendMsgEvent = NULL;
    cStdDev delayStats;
    cOutVector delayVector;
    cOutVector hopCountVector;

public:
    App();
    ~App() override;

protected:
    void initialize() override;
    void finish() override;
    void handleMessage(cMessage* msg) override;
};

Define_Module(App);

#endif /* APP */

App::App() {
}

App::~App() {
}

void App::initialize() {
    // If interArrivalTime for this node is higher than 0
    // initialize packet generator by scheduling sendMsgEvent
    if (par("interArrivalTime").doubleValue() != 0) {
        sendMsgEvent = new cMessage("sendEvent");
        scheduleAt(par("interArrivalTime"), sendMsgEvent);
    }

    // Initialize statistics
    delayStats.setName("TotalDelay");
    delayVector.setName("Delay");
    hopCountVector.setName("HopCount");
}

void App::finish() {
    // Record statistics
    recordScalar("Average delay", delayStats.getMean());
    recordScalar("Number of packets", delayStats.getCount());
}

void App::handleMessage(cMessage* msg) {
    // if msg is a sendMsgEvent, create and send new packet
    if (msg == sendMsgEvent) {
        // create new packet
        Packet* pkt = new Packet("packet", this->getParentModule()->getIndex());
        pkt->setByteLength(par("packetByteSize"));
        pkt->setSource(this->getParentModule()->getIndex());
        pkt->setDestination(par("destination"));

        // send to net layer
        send(pkt, "toNet$o");

        // compute the new departure time and schedule next sendMsgEvent
        simtime_t departureTime = simTime() + par("interArrivalTime");
        scheduleAt(departureTime, sendMsgEvent);
    } else { // else, msg is a packet from net layer
        Packet* pkt = dynamic_cast<Packet*>(msg);

        hopCountVector.record(pkt->getHopCount());

        simtime_t delay = simTime() - pkt->getCreationTime();
        delayStats.collect(delay);
        delayVector.record(delay);
        
        delete pkt;
    }
}
