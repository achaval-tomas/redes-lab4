#ifndef NET
#define NET

#include "DestinationCost.h"
#include "distance_vector_m.h"
#include "hello_m.h"
#include "packet_m.h"
#include <algorithm>
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
    // routingTable[destino, vecino] = costo
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
    void handleDistanceVectorMessage(DistanceVectorMsg* msg);
    bool updateCheapestExitTo(Address destination);
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
    } else if (DistanceVectorMsg* distanceVectorMsg = dynamic_cast<DistanceVectorMsg*>(msg)) {
        handleDistanceVectorMessage(distanceVectorMsg);
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
    } else {
        pkt->setHopCount(pkt->getHopCount() + 1);

        auto destination = pkt->getDestination();
        auto cheapestExit = cheapestExits[destination];
        auto gate = neighbourGates[cheapestExit];
        send(pkt, "toLnk$o", gate);
    }
}

void Net::handleDistanceVectorMessage(DistanceVectorMsg* msg) {
    auto sourceAddress = msg->getSourceAddress();
    bool anyUpdate = false;

    for (size_t i = 0; i < msg->getDistanceVectorArraySize(); i++) {
        auto& destinationCost = msg->getDistanceVector(i);
        auto destination = destinationCost.destination;
        if (destination == nodeAddress) {
            continue;
        }
        auto cost = destinationCost.cost;

        // MAYBE: replace that 1 for neighbourCosts[sourceAddress]
        routingTable[destination][sourceAddress] = cost + 1;
        anyUpdate |= updateCheapestExitTo(destination);
    }

    if (anyUpdate) {
        shareDistanceVector();
    }

    delete msg;
}

void Net::handleHelloMessage(HelloMsg* msg) {
    auto neighbourAddress = msg->getSource();
    auto neighbourGate = msg->getArrivalGate()->getIndex();

    assert(neighbourGates.count(neighbourAddress) == 0);

    neighbourGates[neighbourAddress] = neighbourGate;

    // For now, we just use hops
    Cost cost = 1;

    routingTable[neighbourAddress][neighbourAddress] = cost;

    bool distanceVectorChanged = updateCheapestExitTo(neighbourAddress);
    if (distanceVectorChanged) {
        shareDistanceVector();
    }

    delete msg;
}

bool Net::updateCheapestExitTo(Address destination) {
    auto& allExitsToDestination = routingTable[destination];
    assert(!allExitsToDestination.empty());

    auto& cheapestExit = *std::min_element(
        allExitsToDestination.begin(),
        allExitsToDestination.end(),
        [](const std::pair<Address, Cost>& a, const std::pair<Address, Cost>& b) {
            return a.second <= b.second;
        });

    // Check if cheapest exit for destination has not changed.
    if (cheapestExits.count(destination) == 1 && cheapestExits.at(destination) == cheapestExit.first) {
        return false;
    }

    cheapestExits[destination] = cheapestExit.first;

    return true;
}

void Net::shareDistanceVector() {
    auto distanceVector = computeDistanceVector();

    DistanceVectorMsg* distanceVectorMsg = new DistanceVectorMsg();
    distanceVectorMsg->setDistanceVectorArraySize(distanceVector.size());
    for (size_t i = 0; i < distanceVector.size(); i++) {
        distanceVectorMsg->setDistanceVector(i, distanceVector[i]);
    }
    distanceVectorMsg->setSourceAddress(nodeAddress);

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
