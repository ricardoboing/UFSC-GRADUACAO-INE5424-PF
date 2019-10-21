// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

/* TESTE DE PROTOCOLO */
void teste_protocolo() {
    cout << "Teste de protocolo" << endl;
    char data[1000];

    SOS* sos = new SOS(SOS::PROTOCOL_SOS_1);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND | Protocol: " << SOS::PROTOCOL_SOS_1 << endl;
        
        memset(data, '2', 1000);
        data[1000 - 1] = '\n';

        sos->send(data);
        
    } else {
        cout << "QEMU RECEIVE | Protocol: " << SOS::PROTOCOL_SOS_1  << endl;
        sos->rcv(data);
        cout << "Data: " << data << endl;
    }

    delete sos;
}

int main()
{   
    teste_protocolo();
}