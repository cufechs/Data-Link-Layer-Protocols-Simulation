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

Define_Module(Node);


void Node::initialize()
{
}

void Node::handleMessage(cMessage *msg)
{


    if (msg->isSelfMessage()) { // Host wants to send

        if(strcmp(msg->getName(),SEND_DATA_MSG)==0){ // I need to send a data packet

            bubble("Will send a msg");

            NextFrameToSendTimer_vec.erase(NextFrameToSendTimer_vec.begin());
            delete(msg);

            auto * mPack = new MyPacket();
            mPack->setPayload("just for testing");
            mPack->setType(DATA_AND_ACK);
            mPack->setAckNum(window_pars.frame_expected);
            mPack->setSeqNum(window_pars.next_frame_to_send);
            mPack->setCheckSum(GenerateCheckSumBits(mPack->getPayload()));


            //TODO: Apply framing


            //TODO: Add random error to payload
            applyError_Modification(mPack);


            msg = new cMessage(SEND_DATA_MSG);
            NextFrameToSendTimer_vec.push_back(msg);
            scheduleAt(simTime() + SEND_DATA_WAIT_TIME, msg);

            const char * c = std::to_string(mPack->getSeqNum()).c_str();
            msg = new cMessage(c);
            scheduleAt(simTime() + ACK_TIMEOUT, msg);
            AckTimeOut_vec.push_back(msg);


            if(!applyError_Loss()){
                send(mPack, "outs", PairingWith);

                EV << "Sent to node " << (PairingWith>=getIndex()? PairingWith+1 : PairingWith)
                    << ", Packet Type: DATA_AND_ACK"
                    << ", Sequence number: " << mPack->getSeqNum()
                    << ", Payload: " << mPack->getPayload()
                    << ", Ack number: " << mPack->getAckNum();

                if(applyError_Duplication()){
                    auto * dubPack = mPack->dup();

                    send(dubPack, "outs", PairingWith);

                    EV << ", Packet is duplicated";
                }
            }
            else{
                delete(mPack);

                EV << "Packet lost in its way to node "
                    << (PairingWith>=getIndex()? PairingWith+1 : PairingWith);
            }


            addSlidingWindowParameter(window_pars.next_frame_to_send, 1);
            if(!between(window_pars.Sl, window_pars.next_frame_to_send, window_pars.Sf)) // Don't think it may happen
                addSlidingWindowParameter(window_pars.next_frame_to_send, -1);
        }
        else {
            // Canceling any "to send data" timer
            //for(int i=0; i<(int)NextFrameToSendTimer_vec.size(); i++)
            //    cancelEvent(NextFrameToSendTimer_vec[i]);   //Canceling all planned msg that was to be sent
            //NextFrameToSendTimer_vec.clear();

            for(int i=0; i<(int)AckTimeOut_vec.size(); i++)
                cancelAndDelete(AckTimeOut_vec[i]); //Canceling all ack timeouts timers
            AckTimeOut_vec.clear();

            window_pars.next_frame_to_send = atoi(msg->getName());

            const char * c = std::to_string(window_pars.next_frame_to_send).c_str();
            msg = new cMessage(c);
            scheduleAt(simTime() + ACK_TIMEOUT, msg);
            AckTimeOut_vec.push_back(msg);
        }

    }
    else {
        auto *mPack = check_and_cast<MyPacket *>(msg);

        int packetType = mPack->getType();

        if(packetType == DATA_AND_ACK){

            if(CheckSumBits(mPack->getPayload(), mPack->getCheckSum())){ // Check msg validity
                if(mPack->getSeqNum() == window_pars.frame_expected){ // check that this is the packet i am waiting for
                    incSlidingWindowParameter(window_pars.frame_expected);
                }
            }


            seq_nr ackReceived = (seq_nr)mPack->getAckNum();

            window_pars.next_frame_to_send = (seq_nr)mPack->getAckNum();

            while(between(window_pars.Sl, ackReceived, window_pars.Sf)){
                incSlidingWindowParameter(window_pars.Sl);
                incSlidingWindowParameter(window_pars.Sf);

                if(AckTimeOut_vec.size() > 0){ // Canceling an ack timeout
                    cancelAndDelete(AckTimeOut_vec[0]);
                    AckTimeOut_vec.erase(AckTimeOut_vec.begin());
                }
            }


            /*EV << "Received at node " << getIndex()
                << ", Packet Type: DATA_AND_ACK"
                << ", Sequence number: " << mPack->getSeqNum()
                << ", Payload: " << mPack->getPayload()
                << ", Ack number: " << mPack->getAckNum()
                << ", number of ack variable: " << numOfAck;*/

            delete(mPack);
        }

        else if(packetType == ACKNOWLEDGE){
        }

        else if(packetType == START_SESSION){

            initParameters(window_pars);

            PairingWith = mPack->getSource();

            msg = new cMessage(SEND_DATA_MSG);
            NextFrameToSendTimer_vec.push_back(msg);
            scheduleAt(simTime() + SEND_DATA_WAIT_TIME + (getIndex()*SEND_DATA_WAIT_TIME/(gateSize("outs")+1)), msg);

            delete(mPack);

            EV << "Received at node " << getIndex()
                << ", Packet Type: START_SESSION"
                << ", from " << PairingWith;

        }
        else if(packetType == END_SESSION){
            mPack->setSource(getIndex());
            send(mPack, "outParent");

            EV << "Received at node " << getIndex()
                << ", Packet Type: End_SESSION";
        }

        else if(packetType == ASSIGN_PAIR){

            EV << "Received at node " << getIndex()
                << ", Packet Type: ASSIGN_PAIR"
                << ", Payload: " << mPack->getSource();

            initParameters(window_pars);

            mPack->setType(START_SESSION);
            PairingWith = mPack->getSource();
            if (PairingWith > getIndex())
                PairingWith--;
            mPack->setSource(getIndex());
            send(mPack, "outs", PairingWith);

            msg = new cMessage(SEND_DATA_MSG);
            NextFrameToSendTimer_vec.push_back(msg);
            scheduleAt(simTime() + SEND_DATA_WAIT_TIME + (getIndex()*SEND_DATA_WAIT_TIME/(gateSize("outs")+1)), msg);
        }
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

void Node::applyError_Modification(MyPacket* pak){
    if((rand() % 100) < par("Modification_prob").doubleValue()){

       std::string mypayload= pak->getPayload();
       int msgSize = mypayload.size();
       int random = rand() % msgSize;

       std::bitset<8> chr(mypayload[random]);
       int random2 = rand() % 8;
       chr[random2] = !chr[random2];

       mypayload[random] = (char)chr.to_ulong();
       pak->setPayload(mypayload);
    }
}

bool Node::applyError_Loss(){
    //return true if the packet will get lost
    return (rand() % 100) < par("Loss_prob").doubleValue();
}

bool Node::applyError_Duplication(){
    //return true if the packet will get lost
    return (rand() % 100) < par("Duplication_prob").doubleValue();
}
