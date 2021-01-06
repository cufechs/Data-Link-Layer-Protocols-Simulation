/*
 * GoBackN.h
 *
 *  Created on: Dec 30, 2020
 *      Author: adham
 */

#ifndef GOBACKN_H_
#define GOBACKN_H_

#include "MyPacket_m.h"


#define MAX_SEQ 15
#define ACK_TIMEOUT 4
//#define WAIT_FOR_DATA_TO_PIGGYBACK_TIMEOUT 1
#define SEND_DATA_WAIT_TIME 0.5

//Self message
#define SEND_DATA_MSG "go send some data"
#define ACK_TIMEOUT_MSG "Ack time out"



typedef unsigned int seq_nr;


enum PacketType {DATA_AND_ACK, ACKNOWLEDGE, START_SESSION, END_SESSION, ASSIGN_PAIR};

struct SlidWind_Pars{
    //sender sliding window parameters
    seq_nr Sf;
    seq_nr next_frame_to_send;
    seq_nr Sl;

    //receiver sliding window parameter
    seq_nr frame_expected;
};

static void initParameters(struct SlidWind_Pars &pars){
    pars.next_frame_to_send = 0;
    pars.Sl = 0;
    pars.Sf = MAX_SEQ - 1;

    pars.frame_expected = 0;
}

static bool between(seq_nr a, seq_nr b, seq_nr c){
    //return true if a<=b<c circularly
    return ((a<=b && b<c) || (c<a && a<=b) || (b<c && c<a));
}

static void addSlidingWindowParameter(seq_nr &f, int val){
    f = (seq_nr)((unsigned int)f + val) % (MAX_SEQ+1);
}

static void incSlidingWindowParameter(seq_nr &f){
    f = (seq_nr)(f + 1) % (MAX_SEQ+1);
}

#endif /* GOBACKN_H_ */
