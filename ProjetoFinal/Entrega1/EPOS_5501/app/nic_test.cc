// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

void teste_protocolo() {
    cout << "NIC Test" << endl;
    char data[1000];

    SOS* sos = new SOS();

    if(SOS::mac()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SENDER" << endl;
        
        for(int i = 0; i < 10; i++) {
            memset(data, '2', 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
        }
    } else {
        cout << "QEMU RECEIVER" << endl;
        sos->rcv(data);
        cout << "Data: " << data << endl;
    }

    sos->statistics();

    delete sos;
}

int main()
{
    teste_protocolo();
}
