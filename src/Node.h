//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

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

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

    std::bitset<8> GenerateCheckSumBits(std::string payload);
    bool CheckSumBits(std::string payload, std::bitset<8> chr);

    void applyError_Modification(MyPacket* pak);
    bool applyError_Loss();
    bool applyError_Duplication();
};

#endif
