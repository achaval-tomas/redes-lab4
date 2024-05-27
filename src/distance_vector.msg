cplusplus {{
#include "DestinationCost.h"
}}

class DestinationCost {
    @existingClass;
};

message DistanceVectorMsg {
    DestinationCost distanceVector[];
    unsigned int sourceAddress;
}