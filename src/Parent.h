#ifndef PARENT_H_
#define PARENT_H_

#include <omnetpp.h>
#include <vector>
#include "GoBackN.h"
#include <time.h>
using namespace omnetpp;


class Parent : public cSimpleModule
{
private:
    std::vector<bool> freeNodes;
    int freeNodesCount;
    int NumOfNodes;

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif /* PARENT_H_ */
