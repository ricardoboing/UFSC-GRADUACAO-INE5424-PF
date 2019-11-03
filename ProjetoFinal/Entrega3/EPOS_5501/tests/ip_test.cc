// EPOS IP Protocol Test Program

#include <utility/random.h>
#include <communicator.h>

using namespace EPOS;

const int ITERATIONS = 3;
const int PDU = 2000;

OStream cout;

int teste(NIC<Ethernet> * nic)
{
    void* a = reinterpret_cast<void *>("a");
    //nic->send(const Address & dst, const Protocol & prot, const void * data, unsigned int size);
    nic->send(0x123456, NULL, a, sizeof(a));
    cout << "c";
}
int teste2(NIC<Ethernet> * nic)
{
    void* a;
    //nic->receive(Address * src, Protocol * prot, void * data, unsigned int size);
    nic->receive(0x123456, NULL, a, sizeof(a));
    cout << "b";
}

int main()
{
    cout << "IP Test Program" << endl;
    cout << "Sizes:" << endl;
    cout << "  NIC::Header => " << sizeof(Ethernet::Header) << endl;
    cout << "  IP::Header => " << sizeof(IP::Header) << endl;
    cout << "  UDP::Header => " << sizeof(UDP::Header) << endl;

    IP * ip = IP::get_by_nic(0);

    NIC<Ethernet> * nic = ip->nic();
    teste(nic);
    teste2(nic);

    Alarm::delay(2000000);
    teste(nic);

    cout << "The end!" << endl;

    return 0;
}
