#include "Parent.h"
Define_Module(Parent);

void Parent::initialize()
{
    srand(time(0));

    NumOfNodes = (int)gateSize("outs");

    freeNodesCount = NumOfNodes;

    for(int i=0; i< (int)gateSize("outs"); i++)
        freeNodes.push_back(true);


    for(int i=(int)freeNodes.size()-1; i>=0; i--){
        if(freeNodes[i] == false) // this node is already assigned
            continue;
        if(freeNodesCount < 2)
            break;

        freeNodesCount-=2;
        freeNodes[i] = false;

        int random = rand() % NumOfNodes; //random from 0 to n
        do { //Avoid sending to yourself or assigned node
            random = rand() % NumOfNodes;
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

    srand(time(0));

    if (msg->isSelfMessage()) {
        if(!((rand() % 100) < par("ProbabiltyToNoAssign").doubleValue())){

            for(int i=0; i<NumOfNodes; i++){
                if(freeNodes[i] == false) // this node is already assigned
                    continue;
                if(freeNodesCount < 2)
                    break;

                freeNodesCount-=2;
                freeNodes[i] = false;

                int random = rand() % NumOfNodes; //random from 0 to n
                do { //Avoid sending to yourself or assigned node
                    random = rand() % NumOfNodes;
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
        freeNodes[mPack->getSource()] = true;
        freeNodesCount++;

        bubble((std::to_string(mPack->getSource()) + " is free, Okay").c_str());

        delete msg;
    }
}
