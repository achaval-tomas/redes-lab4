#ifndef NET
#define NET

#include "DestinationCost.h"
#include "distance_vector_m.h"
#include "hello_m.h"
#include "packet_m.h"
#include <omnetpp.h>
#include <string.h>
#include <unordered_map>

using namespace omnetpp;

typedef unsigned int Address;
typedef unsigned int Cost;
typedef unsigned int Gate;

class Net : public cSimpleModule {
private:
    Address nodeAddress = -1;
    // Maps a *neighbour* address to its gate.
    std::unordered_map<Address, Gate> neighbourGates;
    // bestPaths[d] = v such that routingTable[d, v] is the lowest value in its row.
    // Maps any destination address to the gate we should send a packet destined to it.
    std::unordered_map<Address, Address> cheapestExits;
    // routingTable[destino, vecino] = cost
    // Maps a destination address to all the known costs via all the known neighbours.
    std::unordered_map<Address, std::unordered_map<Address, Cost>> routingTable;

public:
    Net();
    ~Net() override;

protected:
    void initialize() override;
    void finish() override;
    void handleMessage(cMessage* msg) override;
    void handleDataPkt(Packet* pkt);
    void handleHelloMessage(HelloMsg* msg);
    // void computeDistanceVector(Address to, Address via, Cost cost);
};

Define_Module(Net);

#endif /* NET */

Net::Net() {
}

Net::~Net() {
}

void Net::initialize() {
    nodeAddress = this->getParentModule()->getIndex();

    int outCount = gateSize("toLnk$o");
    for (int i = 0; i < outCount; i++) {
        auto helloMsg = new HelloMsg();
        helloMsg->setSource(nodeAddress);
        send(helloMsg, "toLnk$o", i);
    }
}

void Net::finish() {
}

void Net::handleMessage(cMessage* msg) {
    // All msg (events) on net are packets
    if (Packet* pkt = dynamic_cast<Packet*>(msg)) {
        handleDataPkt(pkt);
    } else if (HelloMsg* hello = dynamic_cast<HelloMsg*>(msg)) {
        handleHelloMessage(hello);
    } else {
        EV_ERROR << "[NET] unknown message type" << std::endl;
    }
}

void Net::handleDataPkt(Packet* pkt) {
    // If this node is the final destination, send to App
    if (pkt->getDestination() == this->getParentModule()->getIndex()) {
        send(pkt, "toApp$o");
    }
    // If not, forward the packet to some else... to who?
    else {
        // We send to link interface #0, which is the
        // one connected to the clockwise side of the ring
        // Is this the best choice? are there others?
        pkt->setHopCount(pkt->getHopCount() + 1);
        send(pkt, "toLnk$o", 0);
    }
}

void Net::handleHelloMessage(HelloMsg* msg) {
    auto neighbourAddress = msg->getSource();
    auto neighbourGate = msg->getArrivalGate()->getIndex();

    assert(neighbourGates.count(neighbourAddress) == 0);

    neighbourGates[neighbourAddress] = neighbourGate;

    // For now, we just use hops
    Cost cost = 1;

    routingTable[neighbourAddress][neighbourAddress] = cost;
}

// void Net::computeDistanceVector(Address destination, Cost cost) {
//     if (cost < routingTable[destination][cheapestExits[destination]]){
//         routingTable[destination][heapestExits[destination]]
//     }


//     routingTable[to][via] = std::min(cost, routingTable.at(neighbourAddress));

// }

// plan for distance vector routing
// hello.msg -> will be used to know the net's index
// distance_vector.msg -> will be used to send DV

// le mando a mis vecinos un hello
// cuando recibo un hello, me fijo su índice y ese es un vecino mío, borro el paquete
// cuando detecto un vecino nuevo, actualizo mi DV y se lo mando a mis vecinos