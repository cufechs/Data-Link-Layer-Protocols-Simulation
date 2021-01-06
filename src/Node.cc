#include "Node.h"

Define_Module(Node);


void Node::initialize()
{
    InDuty = false;
    bufferSize = 0;

    for(int i=0; i<=MAX_SEQ; i++){
        msg_buffer.push_back("");
        receiving_buffer.push_back("");
    }
}

void Node::handleMessage(cMessage *msg)
{

    if (msg->isSelfMessage()) { // Host wants to send

        if(!InDuty){
            delete(msg);
            return;
        }


        if(strcmp(msg->getName(),SEND_DATA_MSG)==0){ // I need to send a data packet
            //==================================================//
            //=======           Sending Data Packet      =======//
            //==================================================//

            //bubble("Will send a msg");
            //Debuging
            std::string s11 = "Sl: " + std::to_string(window_pars.Sl) + ", Sf: " + std::to_string(window_pars.Sf);
            const char* trwe = s11.c_str();
            bubble(trwe);


            NextFrameToSendTimer_vec.erase(NextFrameToSendTimer_vec.begin());
            delete(msg);

            auto * mPack = new MyPacket();
            mPack->setType(DATA_AND_ACK);
            mPack->setAckNum(window_pars.frame_expected);
            mPack->setSeqNum(window_pars.next_frame_to_send);


            if(bufferSize > 0){
                mPack->setPayload(msg_buffer[window_pars.next_frame_to_send]);
                mPack->setCheckSum(GenerateCheckSumBits(msg_buffer[window_pars.next_frame_to_send]));
            }
            else{   // File data is all sent, Ending session

                InDuty = false;

                for(int i=0; i<(int)NextFrameToSendTimer_vec.size(); i++)
                    cancelAndDelete(NextFrameToSendTimer_vec[i]);

                for(int i=0; i<(int)AckTimeOut_vec.size(); i++)
                    cancelAndDelete(AckTimeOut_vec[i]);

                NextFrameToSendTimer_vec.clear();
                AckTimeOut_vec.clear();

                mPack->setSource(getIndex());
                send(mPack, "outParent");

                auto EndSesPack = new MyPacket();
                EndSesPack->setType(END_SESSION);
                send(EndSesPack, "outs", PairingWith);

                file.close();

                return;
            }


            //TODO: Apply framing


            applyError_Modification(mPack);

            msg = new cMessage(SEND_DATA_MSG);
            NextFrameToSendTimer_vec.push_back(msg);
            scheduleAt(simTime() + SEND_DATA_WAIT_TIME, msg);

            const char * c = std::to_string(mPack->getSeqNum()).c_str();
            msg = new cMessage(c);
            scheduleAt(simTime() + ACK_TIMEOUT, msg);
            AckTimeOut_vec.push_back(msg);


            if(!applyError_Loss()){

                EV << "Sent to node " << (PairingWith>=getIndex()? PairingWith+1 : PairingWith)
                    << ", Packet Type: DATA_AND_ACK"
                    << ", Sequence number: " << mPack->getSeqNum()
                    << ", Ack number: " << mPack->getAckNum()
                    << "\nPayload: " << mPack->getPayload();


                if(applyError_Delay()){
                    double delay = Error_delayTime();
                    sendDelayed(mPack, delay, "outs", PairingWith);
                    EV << ", Packet is delayed by " << delay;
                }
                else
                    send(mPack, "outs", PairingWith);

                if(applyError_Duplication()){
                    auto * dubPack = mPack->dup();

                    send(dubPack, "outs", PairingWith);

                    EV << ", Packet is duplicated";
                }
            }
            else{
                delete(mPack);

                EV << "Packet lost in its way to node "
                    << (PairingWith>=getIndex()? PairingWith+1 : PairingWith) << "\n";
            }


            incSlidingWindowParameter(window_pars.next_frame_to_send);
            if(!between(window_pars.Sl, window_pars.next_frame_to_send, window_pars.Sf)) // Don't think it may happen
                addSlidingWindowParameter(window_pars.next_frame_to_send, -1);
        }
        else {
            //==================================================//
            //=======         Acknowledge timeout        =======//
            //==================================================//

            // Canceling any "to send data" timer
            //for(int i=0; i<(int)NextFrameToSendTimer_vec.size(); i++)
            //    cancelEvent(NextFrameToSendTimer_vec[i]);   //Canceling all planned msg that was to be sent
            //NextFrameToSendTimer_vec.clear();

            for(int i=0; i<(int)AckTimeOut_vec.size(); i++)
                cancelAndDelete(AckTimeOut_vec[i]); //Canceling all ack timeouts timers
            AckTimeOut_vec.clear();

            window_pars.next_frame_to_send = atoi(msg->getName());

            EV << "Ack time out, resending packet of index " << window_pars.next_frame_to_send << "\n";

            const char * c = std::to_string(window_pars.next_frame_to_send).c_str();
            msg = new cMessage(c);
            scheduleAt(simTime() + ACK_TIMEOUT, msg);
            AckTimeOut_vec.push_back(msg);
        }

    }
    else {
        //==================================================//
        //=======         Receiving a Packet         =======//
        //==================================================//


        auto *mPack = check_and_cast<MyPacket *>(msg);

        int packetType = mPack->getType();

        if(packetType == DATA_AND_ACK){
            //==================================================//
            //=======       Receiving Data Packet        =======//
            //==================================================//

            if(CheckSumBits(mPack->getPayload(), mPack->getCheckSum())){ // Check msg validity
                if(mPack->getSeqNum() == window_pars.frame_expected){ // check that this is the packet i am waiting for
                    receiving_buffer[window_pars.frame_expected] = mPack->getPayload();
                    incSlidingWindowParameter(window_pars.frame_expected);
                    bubble("Thanks!");
                }
            }


            seq_nr ackReceived = (seq_nr)mPack->getAckNum();

            window_pars.next_frame_to_send = (seq_nr)mPack->getAckNum();

			addSlidingWindowParameter(ackReceived, -1);

            while(between(window_pars.Sl, ackReceived, window_pars.Sf)){
                incSlidingWindowParameter(window_pars.Sl);
                incSlidingWindowParameter(window_pars.Sf);

                bufferSize--;

                std::string line;
                if(std::getline(file, line)){ // Adding data to the buffer
                    msg_buffer[window_pars.Sf] = line;
                    bufferSize++;
                }

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

            InDuty = true;

            do { // Opening text file
                int r = (rand() % 32) + 1;
                std::string dir = "C:/Users/adham/OneDrive/Desktop/Data_files/File_" + std::to_string(r) + ".txt";
                file.open(dir.c_str());
            } while (!file.is_open());

            /*do { // Opening text file
                int r = getIndex();
                std::string dir = "C:/Users/adham/OneDrive/Desktop/Data_files/" + std::to_string(r) + ".txt";
                file.open(dir.c_str());
            } while (!file.is_open());*/

            initParameters(window_pars);

            std::string line;
            seq_nr temp_ptr = window_pars.next_frame_to_send;
            while(between(window_pars.Sl, temp_ptr, window_pars.Sf)){
                if(std::getline(file, line)){
                    msg_buffer[temp_ptr] = line;
                    bufferSize++;
                }
                incSlidingWindowParameter(temp_ptr);
            }

            PairingWith = mPack->getSource();
            if (PairingWith > getIndex())
                PairingWith--;

            msg = new cMessage(SEND_DATA_MSG);
            NextFrameToSendTimer_vec.push_back(msg);
            scheduleAt(simTime() + SEND_DATA_WAIT_TIME + (getIndex()*SEND_DATA_WAIT_TIME/(gateSize("outs")+1)), msg);

            delete(mPack);

            EV << "Received at node " << getIndex()
                << ", Packet Type: START_SESSION"
                << ", from " << PairingWith;

        }
        else if(packetType == END_SESSION){

            InDuty = false;
            file.close();
            bufferSize = 0;

            for(int i=0; i<(int)NextFrameToSendTimer_vec.size(); i++)
                cancelAndDelete(NextFrameToSendTimer_vec[i]);

            for(int i=0; i<(int)AckTimeOut_vec.size(); i++)
                cancelAndDelete(AckTimeOut_vec[i]);

            NextFrameToSendTimer_vec.clear();
            AckTimeOut_vec.clear();

            mPack->setSource(getIndex());
            send(mPack, "outParent");

            EV << "Received at node " << getIndex()
                << ", Packet Type: End_SESSION";
        }
        else if(packetType == ASSIGN_PAIR){

            InDuty = true;

            do { // Opening text file
                int r = (rand() % 32) + 1;
                std::string dir = "C:/Users/adham/OneDrive/Desktop/Data_files/File_" + std::to_string(r) + ".txt";
                file.open(dir.c_str());
            } while (!file.is_open());

            /*do { // Opening text file
                int r = getIndex();
                std::string dir = "C:/Users/adham/OneDrive/Desktop/Data_files/" + std::to_string(r) + ".txt";
                file.open(dir.c_str());
            } while (!file.is_open());*/

            initParameters(window_pars);

            std::string line;
            seq_nr temp_ptr = window_pars.next_frame_to_send;
            while(between(window_pars.Sl, temp_ptr, window_pars.Sf)){
                if(std::getline(file, line)){
                    msg_buffer[temp_ptr] = line;
                    bufferSize++;
                }
                incSlidingWindowParameter(temp_ptr);
            }


            EV << "Received at node " << getIndex()
                << ", Packet Type: ASSIGN_PAIR"
                << ", Payload: " << mPack->getSource();



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
        if(chr[8]){
            chr[8] = 0;
            temp = (int)(chr.to_ulong()) + 1;
            chr = std::bitset<9>(temp);
        }
    }
    std::bitset<8> out;
    for(int i=0; i<8; i++) //Todo: update this
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

bool Node::applyError_Delay(){
    //return true if the packet will get delayed
    return (rand() % 100) < par("Delays_prob").doubleValue();
}

double Node::Error_delayTime(){
    //return the delay time, where  0 < delay < Max_Delay_time

    double f = (double)rand() / RAND_MAX;
    return f * par("Max_Delay_time").doubleValue();
}
