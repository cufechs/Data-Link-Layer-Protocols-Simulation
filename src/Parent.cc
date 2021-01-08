#include "Parent.h"
Define_Module(Parent);

void Parent::initialize()
{
    srand(time(0)); // Randomization related

    generatedFrames_count = 0;
    droppedFrames_count = 0;
    retransmittedFrames_count = 0;
    usefulFrames_count = 0;

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
        if(!((rand() % 100) < par("ProbabiltyToNotAssign").doubleValue())){

            for(int i=0; i<NumOfNodes; i++){
                if(freeNodes[i] == false) // this node is already assigned
                    continue;
                if(freeNodesCount < 2) // no more nodes to pairs to assign?
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
        scheduleAt(simTime() + par("refreshGap_TimeStep").doubleValue(), new cMessage(""));

        //Printing stats
        EV << "The total number of generated frames is " << generatedFrames_count << "\n"
                << "The total number of dropped frames is " << droppedFrames_count << "\n"
                << "The total number of retransmitted frames is " << retransmittedFrames_count << "\n"
                << "The total number of useful data transmitted is " << usefulFrames_count << "\n"
                << "Percentage of useful data transmitted is " << ((double)usefulFrames_count/generatedFrames_count) * 100 << "%\n";

    }
    else{
        auto *mPack = check_and_cast<MyPacket *>(msg);
        if(mPack->getType() == END_SESSION){
            freeNodes[mPack->getSource()] = true;
            freeNodesCount++;

            bubble((std::to_string(mPack->getSource()) + " is free, Okay").c_str());
        }
        else if(mPack->getType() == STATS_TYPE1){
            generatedFrames_count += mPack->getAckNum();
            droppedFrames_count += mPack->getSeqNum();
        }
        else if(mPack->getType() == STATS_TYPE2){
            retransmittedFrames_count += mPack->getAckNum();
            usefulFrames_count += mPack->getSeqNum();
        }



        delete msg;
    }
}
