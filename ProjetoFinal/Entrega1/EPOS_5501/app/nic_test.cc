// EPOS NIC Test Programs

#include <machine/nic.h>
#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;
int teste1(NIC<Ethernet>::Address self, SOS* sos){
    
    char data[1000];
    if(self[5] % 2) { // sender
        Delay (5000000);
        cout << "sender" << endl;
        //for(int i = 0; i < 10; i++) {
            memset(data, '1', 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
        //}
    } else { // receiver
        cout << "receiver" << endl;
        sos->rcv(data);
        cout << " data " << data << endl;
    }
    return 0;
}


int main()
{
    cout << "NIC Test" << endl;
    NIC<Ethernet> * nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    NIC<Ethernet>::Address self = nic->address();

    SOS* sos = SOS::ponteiro;

    teste1(self, sos);

    sos->statistics();

    delete sos;
    Delay (500000000);
}
