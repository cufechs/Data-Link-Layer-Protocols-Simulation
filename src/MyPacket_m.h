//
// Generated file, do not edit! Created by nedtool 5.6 from MyPacket.msg.
//

#ifndef __MYPACKET_M_H
#define __MYPACKET_M_H

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#include <omnetpp.h>

// nedtool version check
#define MSGC_VERSION 0x0506
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of nedtool: 'make clean' should help.
#endif



// cplusplus {{
    #include <bitset>  
	typedef std::bitset<8> bits;
// }}

/**
 * Class generated from <tt>MyPacket.msg:25</tt> by nedtool.
 * <pre>
 * //
 * //
 * packet MyPacket
 * {
 *     int seqNum;
 *     int type;
 *     string payload;
 *     int AckNum;
 *     int source;
 *     bits checkSum;
 * }
 * </pre>
 */
class MyPacket : public ::omnetpp::cPacket
{
  protected:
    int seqNum;
    int type;
    ::omnetpp::opp_string payload;
    int AckNum;
    int source;
    bits checkSum;

  private:
    void copy(const MyPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const MyPacket&);

  public:
    MyPacket(const char *name=nullptr, short kind=0);
    MyPacket(const MyPacket& other);
    virtual ~MyPacket();
    MyPacket& operator=(const MyPacket& other);
    virtual MyPacket *dup() const override {return new MyPacket(*this);}
    virtual void parsimPack(omnetpp::cCommBuffer *b) const override;
    virtual void parsimUnpack(omnetpp::cCommBuffer *b) override;

    // field getter/setter methods
    virtual int getSeqNum() const;
    virtual void setSeqNum(int seqNum);
    virtual int getType() const;
    virtual void setType(int type);
    virtual const char * getPayload() const;
    virtual void setPayload(std::string payload);
    virtual int getAckNum() const;
    virtual void setAckNum(int AckNum);
    virtual int getSource() const;
    virtual void setSource(int source);
    virtual bits& getCheckSum();
    virtual const bits& getCheckSum() const {return const_cast<MyPacket*>(this)->getCheckSum();}
    virtual void setCheckSum(const bits& checkSum);
};

inline void doParsimPacking(omnetpp::cCommBuffer *b, const MyPacket& obj) {obj.parsimPack(b);}
inline void doParsimUnpacking(omnetpp::cCommBuffer *b, MyPacket& obj) {obj.parsimUnpack(b);}


#endif // ifndef __MYPACKET_M_H

