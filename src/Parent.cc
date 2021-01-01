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

#include "Parent.h"
Define_Module(Parent);

void Parent::initialize()
{
    srand(time(0));

    freeNodesCount = (int)gateSize("outs");

    for(int i=0; i< (int)gateSize("outs"); i++)
        freeNodes.push_back(true);

    std::vector<int> toErase;

    for(int i=0; i< (int)freeNodes.size(); i++){
        if(freeNodes[i] == false) // this node is already assigned
            continue;
        if(freeNodesCount < 2)
            break;

        freeNodesCount-=2;
        freeNodes[i] = false;

        int random = rand() % gateSize("outs"); //random from 0 to n
        do { //Avoid sending to yourself or assigned node
            random = rand() % gateSize("outs");
        } while(!freeNodes[random]);

        freeNodes[random] = false;

        auto * mPack = new MyPacket();
        mPack->setSource(random);
        mPack->setType(ASSIGN_PAIR);

        send(mPack, "outs", i);
    }

    double interval = par("refreshGap_TimeStep").doubleValue();
    scheduleAt(simTime() + interval, new cMessage(""));
}

void Parent::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if((rand() % 100) < par("ProbabiltyToNoAssign").doubleValue()){
            for(int i=0; i<(int)freeNodes.size(); i++){
                if(freeNodes[i] == false) // this node is already assigned
                    continue;
                if(freeNodesCount < 2)
                    break;

                freeNodesCount-=2;
                freeNodes[i] = false;

                int random = rand() % gateSize("outs"); //random from 0 to n
                do { //Avoid sending to yourself or assigned node
                    random = rand() % gateSize("outs");
                } while(!freeNodes[random]);

                freeNodes[random] = false;

                auto * mPack = new MyPacket();
                mPack->setSource(random);
                mPack->setType(ASSIGN_PAIR);

                send(mPack, "outs", i);
            }
        }

        delete msg;
        double interval = par("refreshGap_TimeStep").doubleValue();
        scheduleAt(simTime() + interval, new cMessage(""));
    }
    else{
        auto *mPack = check_and_cast<MyPacket *>(msg);
        freeNodes[atoi(mPack->getPayload())] = true;
        freeNodesCount++;
        delete msg;
    }
}
