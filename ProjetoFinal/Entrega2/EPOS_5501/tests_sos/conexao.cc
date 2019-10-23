// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

/* TESTE DE CONEXAO */
void teste_conexao() {
    cout << "Teste de conexao" << endl;
    char data[10];

    SOS* sos = new SOS(8989);
    
    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND | Protocol: "  << endl;
        memset(data, '1', 10);
        data[1000 - 1] = '\n';
            
        sos->send(data);
    } else {
        cout << "QEMU RECEIVE | Protocol: "  << endl;
        sos->receive(data,10);
        cout << "Data: " << data << endl;
    }

    delete sos;
}


int main()
{   
    teste_conexao();
    Delay (500000000);
}