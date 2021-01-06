#ifndef __SECTION1_NODE_H_
#define __SECTION1_NODE_H_

#include <omnetpp.h>
#include <bitset>
#include <vector>
#include <fstream>
#include <sstream>
#include "GoBackN.h"
using namespace omnetpp;



class Node : public cSimpleModule
{
private:
    std::ifstream file;
    SlidWind_Pars window_pars;
    int PairingWith;
    std::vector<cMessage *> AckTimeOut_vec;
    std::vector<cMessage *> NextFrameToSendTimer_vec;
    bool InDuty;
    std::vector<std::string> msg_buffer;
    std::vector<std::string> receiving_buffer;
    int bufferSize;

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    std::bitset<8> GenerateCheckSumBits(std::string payload);
    bool CheckSumBits(std::string payload, std::bitset<8> chr);

    void applyError_Modification(MyPacket* pak);
    bool applyError_Loss();
    bool applyError_Duplication();
    bool applyError_Delay();
    double Error_delayTime();
};

#endif
