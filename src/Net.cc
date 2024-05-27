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
    bool updateCheapestExitTo(Address destination, Address via, Cost cost);
    void shareDistanceVector();
    std::vector<DestinationCost> computeDistanceVector();
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

    bool distanceVectorChanged = updateCheapestExitTo(neighbourAddress, neighbourAddress, cost);
    if (distanceVectorChanged) {
        shareDistanceVector();
    }
}

bool Net::updateCheapestExitTo(Address destination, Address via, Cost cost) {
    if (cheapestExits.count(destination) == 0) {
        cheapestExits[destination] = via;
        return true;
    }
    Address currentExit = cheapestExits[destination];

    assert(routingTable[destination].count(currentExit) == 1);

    Cost currentCost = routingTable[destination][currentExit];

    if (cost < currentCost) {
        cheapestExits[destination] = via;
        return true;
    }

    return false;
}

void Net::shareDistanceVector() {
    auto distanceVector = computeDistanceVector();

    DistanceVectorMsg* distanceVectorMsg = new DistanceVectorMsg();
    distanceVectorMsg->setDistanceVectorArraySize(distanceVector.size());
    for (size_t i = 0; i < distanceVector.size(); i++) {
        distanceVectorMsg->setDistanceVector(i, distanceVector[i]);
    }

    for (auto& addressAndGate : neighbourGates) {
        auto gate = addressAndGate.second;

        send(distanceVectorMsg->dup(), "toLnk$o", gate);
    }

    delete distanceVectorMsg;
}

std::vector<DestinationCost> Net::computeDistanceVector() {
    auto distanceVector = std::vector<DestinationCost>();

    for (auto& destinationAndVia : cheapestExits) {
        auto& destination = destinationAndVia.first;
        auto& via = destinationAndVia.second;

        auto cost = routingTable[destination][via];
        distanceVector.push_back(DestinationCost { destination, cost });
    }

    return distanceVector;
}

// plan for distance vector routing
// hello.msg -> will be used to know the net's index
// distance_vector.msg -> will be used to send DV

// le mando a mis vecinos un hello
// cuando recibo un hello, me fijo su índice y ese es un vecino mío, borro el paquete
// cuando detecto un vecino nuevo, actualizo mi DV y se lo mando a mis vecinos