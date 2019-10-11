// EPOS NIC Test Programs

#include <machine/nic.h>
#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

int main()
{
    cout << "NIC Test" << endl;
    NIC<Ethernet> * nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    NIC<Ethernet>::Address self = nic->address();

    char data[1000];

    SOS* sos = SOS::ponteiro;

    if(self[5] % 2) { // sender
        Delay (5000000);
        cout << "1" << endl;
        for(int i = 0; i < 10; i++) {
            memset(data, '2', 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
        }
    } else { // receiver
        cout << "2" << endl;
        sos->rcv(data);
        cout << " data " << data << endl;
    }

    sos->statistics();

    delete sos;
}
