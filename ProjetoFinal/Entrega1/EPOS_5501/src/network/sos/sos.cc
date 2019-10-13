// EPOS IP Protocol Implementation

#include <utility/string.h>
#include <network/sos/sos.h>
#include <system.h>

#ifdef __sos__

__BEGIN_SYS
/*
NIC<Ethernet>* SOS::nic;

SOS::SOS() {
    using namespace EPOS;
    OStream cout;
    cout << "SOS::SOS" << endl;

    SOS::nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    SOS::nic->attach(this, 0x8888);
    
    _semaphore = new Semaphore(0);
}
SOS::~SOS() {
    using namespace EPOS;
    OStream cout;
    cout << "SOS::~SOS" << endl;

    SOS::nic->detach(this, 0x8888);
}

void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Buffer * buf)
{
    using namespace EPOS;
    OStream cout;
    cout << "SOS::update | _semaphore->v" << endl;

    _semaphore->v();
}

void SOS::send(char data[]) {
    using namespace EPOS;
    OStream cout;
    cout << "SOS::send" << endl;

    SOS::nic->send(nic->broadcast(), 0x8888, data, nic->mtu());
}
void SOS::rcv(char data[]) {
    using namespace EPOS;
    OStream cout;
    cout << "SOS::rcv | entrou _semaphore->p" << endl;

    _semaphore->p();

    cout << "SOS::rcv | saiu _semaphore->p" << endl;

    NIC<Ethernet>::Address src;
    NIC<Ethernet>::Protocol prot;

    SOS::nic->receive(&src, &prot, data, 1000);
}
void SOS::statistics() {
    using namespace EPOS;
    OStream cout;
    NIC<Ethernet>::Statistics stat = SOS::nic->statistics();

    cout << "Statistics\n"
         << "Tx Packets: " << stat.tx_packets << "\n"
         << "Tx Bytes:   " << stat.tx_bytes << "\n"
         << "Rx Packets: " << stat.rx_packets << "\n"
         << "Rx Bytes:   " << stat.rx_bytes << "\n";
}
*/
__END_SYS

#endif
