#include "Node.h"

Define_Module(Node);


void Node::initialize()
{
    // Stats related
    scheduleAt(par("Send_stat_time").doubleValue() - 0.0000001, new cMessage(STATS_MSG));


    InDuty = false;
    bufferSize = 0;
    generatedFrames_count = 0;
    droppedFrames_count = 0;
    retransmittedFrames_count = 0;
    usefulFrames_count = 0;

    for(int i=0; i<=MAX_SEQ; i++){
        msg_buffer.push_back(std::make_pair("", false));
        receiving_buffer.push_back("");
    }
}

void Node::handleMessage(cMessage *msg)
{

    if (msg->isSelfMessage()) { // Host wants to send


        if(strcmp(msg->getName(),STATS_MSG)==0){ // Send stats to parent
            //AckNum and SeqNum data members are used for other purposes

            delete(msg);

            auto * mPack = new MyPacket();
            mPack->setType(STATS_TYPE1);
            mPack->setSource(getIndex());
            mPack->setAckNum(generatedFrames_count);
            mPack->setSeqNum(droppedFrames_count);
            send(mPack, "outParent");

            mPack = new MyPacket();
            mPack->setType(STATS_TYPE2);
            mPack->setSource(getIndex());
            mPack->setAckNum(retransmittedFrames_count);
            mPack->setSeqNum(usefulFrames_count);
            send(mPack, "outParent");

            // resetting counts
            generatedFrames_count = 0;
            droppedFrames_count = 0;
            retransmittedFrames_count = 0;
            usefulFrames_count = 0;

            scheduleAt(simTime() + par("Send_stat_time").doubleValue(), new cMessage(STATS_MSG));

            return;
        }


        // If the node in not in duty, then it is not expecting a self msg
        if(!InDuty){
            delete(msg);
            return;
        }


        if(strcmp(msg->getName(),SEND_DATA_MSG)==0){ // I need to send a data packet
            //==================================================//
            //=======           Sending Data Packet      =======//
            //==================================================//


            //Debuging
            bubble("Will send a msg");
            //std::string s11 = "Sf: " + std::to_string(window_pars.first_frame_in_window)
            //                    + ", Sl: " + std::to_string(window_pars.last_frame_in_window);
            //const char* trwe = s11.c_str();
            //bubble(trwe);


            NextFrameToSendTimer_vec.erase(NextFrameToSendTimer_vec.begin());
            delete(msg);

            auto * mPack = new MyPacket();
            mPack->setType(DATA_AND_ACK);
            mPack->setAckNum(window_pars.frame_expected);
            mPack->setSeqNum(window_pars.next_frame_to_send);


            if(bufferSize > 0){
                mPack->setPayload(msg_buffer[window_pars.next_frame_to_send].first);
                mPack->setCheckSum(GenerateCheckSumBits(msg_buffer[window_pars.next_frame_to_send].first));

                if(msg_buffer[window_pars.next_frame_to_send].second == true) // This frame is being retransmitted
                    retransmittedFrames_count++;
                else
                    msg_buffer[window_pars.next_frame_to_send].second = true;
            }
            else{   // File data is all sent, Ending session

                //==================================================//
                //=======           Ending Session           =======//
                //==================================================//

                InDuty = false;

                for(int i=0; i<(int)NextFrameToSendTimer_vec.size(); i++)
                    cancelAndDelete(NextFrameToSendTimer_vec[i]);

                for(int i=0; i<(int)AckTimeOut_vec.size(); i++)
                    cancelAndDelete(AckTimeOut_vec[i]);

                NextFrameToSendTimer_vec.clear();
                AckTimeOut_vec.clear();

                mPack->setType(END_SESSION);
                mPack->setSource(getIndex());
                send(mPack, "outParent");

                auto EndSesPack = new MyPacket();
                EndSesPack->setType(END_SESSION);
                send(EndSesPack, "outs", PairingWith);

                file.close();

                return;
            }


            //Applying framing
            mPack->setPayload(Framing(mPack->getPayload()));



            // Setting timer
            msg = new cMessage(SEND_DATA_MSG);
            NextFrameToSendTimer_vec.push_back(msg);
            scheduleAt(simTime() + SEND_DATA_WAIT_TIME, msg);

            // Setting Ack timeout timer
            const char * c = std::to_string(mPack->getSeqNum()).c_str();
            msg = new cMessage(c);
            scheduleAt(simTime() + ACK_TIMEOUT, msg);
            AckTimeOut_vec.push_back(msg);


            //==================================================//
            //=======          Sending a Packet          =======//


            applyError_Modification(mPack);

            generatedFrames_count++;

            if(!applyError_Loss()){


                EV << "Sent to node " << (PairingWith>=getIndex()? PairingWith+1 : PairingWith)
                    << ", Packet Type: DATA_AND_ACK"
                    << ", Sequence number: " << mPack->getSeqNum()
                    << ", Ack number: " << mPack->getAckNum()
                    << "\nPayload: " << mPack->getPayload();


                if(applyError_Delay()){ // The packet is delayed
                    double delay = Error_delayTime();
                    sendDelayed(mPack, delay, "outs", PairingWith);
                    EV << ", Packet is delayed by " << delay;
                }
                else
                    send(mPack, "outs", PairingWith);

                if(applyError_Duplication()){ // The packet is duplicated

                    generatedFrames_count++;

                    auto * dubPack = mPack->dup();
                    send(dubPack, "outs", PairingWith);
                    EV << ", Packet is duplicated";
                }
            }
            else{ // Packet is lost through transmission
                delete(mPack);

                EV << "Packet lost in its way to node "
                    << (PairingWith>=getIndex()? PairingWith+1 : PairingWith) << "\n";
            }

            //==================================================//

            incSlidingWindowParameter(window_pars.next_frame_to_send);
            if(!between(window_pars.first_frame_in_window, window_pars.next_frame_to_send, window_pars.last_frame_in_window)) // Don't think it may happen
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


            //Deframing
            mPack->setPayload(Deframing(mPack->getPayload()));

            if(mPack->getSeqNum() == window_pars.frame_expected && CheckSumBits(mPack->getPayload(), mPack->getCheckSum())){    // check that this is the packet that I am waiting
                receiving_buffer[window_pars.frame_expected] = mPack->getPayload();                                             // for (as a node) and Checking msg validity
                incSlidingWindowParameter(window_pars.frame_expected);
                usefulFrames_count++;
                bubble("Thanks! Useful frame :)");
            }
            else
                droppedFrames_count++;


            window_pars.next_frame_to_send = (seq_nr)mPack->getAckNum();

            seq_nr ackReceived = (seq_nr)mPack->getAckNum();
            addSlidingWindowParameter(ackReceived, -1);

            while(between(window_pars.first_frame_in_window, ackReceived, window_pars.last_frame_in_window)){
                incSlidingWindowParameter(window_pars.first_frame_in_window);
                incSlidingWindowParameter(window_pars.last_frame_in_window);

                bufferSize--;

                std::string line;
                if(std::getline(file, line)){ // Adding data to the buffer
                    msg_buffer[window_pars.last_frame_in_window].first = line;
                    msg_buffer[window_pars.last_frame_in_window].second = false;
                    bufferSize++;
                }

                if(AckTimeOut_vec.size() > 0){ // Canceling an ack timeout
                    cancelAndDelete(AckTimeOut_vec[0]);
                    AckTimeOut_vec.erase(AckTimeOut_vec.begin());
                }
            }


            EV << "Received at node " << getIndex()
                << ", Packet Type: DATA_AND_ACK"
                << ", Sequence number: " << mPack->getSeqNum()
                << ", \nPayload: " << mPack->getPayload()
                << ", Ack number: " << mPack->getAckNum();

            delete(mPack);
        }

        else if(packetType == ACKNOWLEDGE){
        }

        else if(packetType == START_SESSION){

            InDuty = true;

            do { // Opening text file
                int r = (rand() % 32) + 1;
                std::string dir = DATA_FILE_DIRECTORY + std::to_string(r) + ".txt";
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
            while(between(window_pars.first_frame_in_window, temp_ptr, window_pars.last_frame_in_window)){
                if(std::getline(file, line)){
                    msg_buffer[temp_ptr].first = line;
                    msg_buffer[temp_ptr].second = false;
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

            mPack->setType(END_SESSION);
            mPack->setSource(getIndex());
            send(mPack, "outParent");

            EV << "Received at node " << getIndex()
                << ", Packet Type: End_SESSION";
        }
        else if(packetType == ASSIGN_PAIR){

            InDuty = true;

            do { // Opening text file
                int r = (rand() % 32) + 1;
                std::string dir = DATA_FILE_DIRECTORY + std::to_string(r) + ".txt";
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
            while(between(window_pars.first_frame_in_window, temp_ptr, window_pars.last_frame_in_window)){
                if(std::getline(file, line)){
                    msg_buffer[temp_ptr].first = line;
                    msg_buffer[temp_ptr].second = false;
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

std::string Node::Framing(std::string mypayload){
    char FLAG = (char)126;
    char ESC = (char)224;
    int counter = 0; //for framing
    int length = mypayload.size();

    for (int i = 0; i < (length + counter); i++)
    {
        if (mypayload[i] == FLAG || mypayload[i] == ESC) //flag
        {
            mypayload.resize(mypayload.size() + 1);
            for (int j = (length + counter); j > i; j--)
            {
                mypayload[j] = mypayload[j - 1];
            }
            mypayload[i] = ESC;
            counter = counter + 1;
            i = i + 1;
        }
    }

    mypayload = FLAG + mypayload + FLAG;

    return mypayload;
}

std::string Node::Deframing(std::string mypayload){
    char FLAG = (char)126;
    char ESC = (char)224;
    int counter = 0;
    int length = mypayload.size();

    for (int i = 0; i < (length - counter); i++)
    {
        if (mypayload[i] == FLAG || mypayload[i] == ESC) //flag
        {
            mypayload.resize(mypayload.size() - 1);
            counter++;
            for (int j = i; j < (length - counter); j++)
                mypayload[j] = mypayload[j + 1];
        }
    }

    return mypayload;
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
