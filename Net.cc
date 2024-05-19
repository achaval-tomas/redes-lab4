#ifndef NET
#define NET

#include <omnetpp.h>
#include <packet_m.h>
#include <string.h>

using namespace omnetpp;

class Net : public cSimpleModule {
private:
public:
    Net();
    ~Net() override;

protected:
    void initialize() override;
    void finish() override;
    void handleMessage(cMessage* msg) override;
};

Define_Module(Net);

#endif /* NET */

Net::Net() {
}

Net::~Net() {
}

void Net::initialize() {
}

void Net::finish() {
}

void Net::handleMessage(cMessage* msg) {
    // All msg (events) on net are packets
    Packet* pkt = dynamic_cast<Packet*>(msg);

    // If this node is the final destination, send to App
    if (pkt->getDestination() == this->getParentModule()->getIndex()) {
        send(msg, "toApp$o");
    }
    // If not, forward the packet to some else... to who?
    else {
        // We send to link interface #0, which is the
        // one connected to the clockwise side of the ring
        // Is this the best choice? are there others?
        send(msg, "toLnk$o", 0);
    }
}
