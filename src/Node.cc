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

#include "Node.h"
#include "MyPacket_m.h"
Define_Module(Node);


void Node::initialize()
{
    double interval = exponential(1 / par("lambda").doubleValue());
    scheduleAt(simTime() + interval, new cMessage(""));
}

void Node::handleMessage(cMessage *msg)
{

    if (msg->isSelfMessage()) { //Host wants to send


        auto * mPack = new MyPacket();
        mPack->setPayload("This is a random string");
        mPack->setType(1); // type data
        mPack->setSeqNum(0);
        mPack->setCheckSum(GenerateCheckSumBits(mPack->getPayload()));


        int rand, dest;
        do { //Avoid sending to yourself
            rand = uniform(0, gateSize("outs"));
        } while(rand == getIndex());

        //Calculate appropriate gate number
        dest = rand;
        if (rand > getIndex())
            dest--;


        mPack->setSource(getIndex());
        mPack->setDestination(dest);

        std::stringstream ss ;
        ss << rand;
        EV << "Sending "<< ss.str() <<" from source " << exponential(1 / par("lambda").doubleValue()) << "\n";
        delete msg;
        send(mPack, "outs", dest);

        double interval = exponential(1 / par("lambda").doubleValue());
        EV << ". Scheduled a new packet after " << interval << "s";
        scheduleAt(simTime() + interval, new cMessage(""));
    }
    else {
        auto *mPack = check_and_cast<MyPacket *>(msg);

        if (CheckSumBits(mPack->getPayload(), mPack->getCheckSum()))
            bubble("Message received");
        else
            bubble("Wrong destination");
        delete msg;
    }
}

std::bitset<8> Node::GenerateCheckSumBits(std::string payload){
    std::bitset<9> chr(0);
    for(int i=0; i<payload.size();i++){
        int temp = (int)(chr.to_ulong()) + payload.at(i);
        chr = std::bitset<9>(temp);
        while(chr[8]){
            chr[8] = 0;
            temp = (int)(chr.to_ulong()) + 1;
            chr = std::bitset<9>(temp);
        }
    }
    std::bitset<8> out;
    for(int i=0; i<8; i++)
        out[i] = !chr[i];

    return out;
}

bool Node::CheckSumBits(std::string payload, std::bitset<8> checksum){
    std::bitset<9> chr(0);
    for(int i=0; i<8; i++)
        chr[i] = checksum[i];
    for(int i=0; i<payload.size();i++){
        int temp = (int)(chr.to_ulong()) + payload.at(i);
        chr = std::bitset<9>(temp);
        while(chr[8]){
            chr[8] = 0;
            temp = (int)(chr.to_ulong()) + 1;
            chr = std::bitset<9>(temp);
        }
    }
    std::bitset<8> out;
    for(int i=0; i<8; i++)
        if(chr[i] == 0)
            return false;

    return true;
}
