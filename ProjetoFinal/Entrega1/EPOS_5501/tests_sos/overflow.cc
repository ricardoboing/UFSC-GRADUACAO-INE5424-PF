// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

#define teste 250

void teste_overflow() {
    cout << "Teste de overflow" << endl;
    char data[1000];

    SOS* sos = new SOS(SOS::PROTOCOL_SOS);
    
    if(SOS::nic_address()[5] % 2) {
        for (int i = 0; i < teste; i++) {
            cout << "QEMU SEND | Protocol: " << SOS::PROTOCOL_SOS << endl;
            memset(data, i % 10, 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
        }
    } else {
        cout << "QEMU RECEIVE | Protocol: " << SOS::PROTOCOL_SOS << endl;
        cout << "Delay para encher o buffer" << endl;
        Delay (5000000);
        cout << "Fim do Delay" << endl << endl;
        for (int i = 0; i < teste; i++) {
            sos->rcv(data);
        }
    }

    SOS::statistics();

    delete sos;
}

int main()
{   
    teste_overflow();
}