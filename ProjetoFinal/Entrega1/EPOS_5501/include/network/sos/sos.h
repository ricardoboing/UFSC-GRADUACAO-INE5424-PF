// EPOS IP Protocol Declarations

#ifndef __sos_h
#define __sos_h

#include <system/config.h>

#ifdef __sos__

#include <utility/bitmap.h>
#include <machine/nic.h>

__BEGIN_SYS
/*
class SOS: private NIC<Ethernet>::Observer {
public:
    typedef unsigned char Protocol;
    typedef Ethernet::Buffer Buffer;
    typedef Data_Observer<Buffer, Protocol> Observer;
    typedef Data_Observed<Buffer, Protocol> Observed;
    typedef NIC<Ethernet>::Address NIC_Address;

    SOS();
    ~SOS();

    void send(char data[]);
    void rcv(char data[]);
    void statistics();

    static void attach(Observer * obs, const Protocol & prot) { _observed.attach(obs, prot); }
    static void detach(Observer * obs, const Protocol & prot) { _observed.detach(obs, prot); }
    
    static NIC_Address mac() { return SOS::nic->address(); }

private:
    void update(Ethernet::Observed * obs, const Ethernet::Protocol & prot, Buffer * buf);

protected:
    static Observed _observed;
    static NIC<Ethernet> * nic; // TSTP static e IP nao
    Semaphore* _semaphore;

};
*/
__END_SYS

#endif
#endif