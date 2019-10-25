// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

/* TESTE DE CONEXAO */
void teste_conexao() {
    cout << "Teste de conexao 1" << endl;
    char data[2000];

    SOS* sos = new SOS(8989);
    
    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND | Protocol: "  << endl;
        memset(data, '1', 2000);
        data[2000 - 1] = '\n';
        //for (int i = 0; i<2;i++){
            sos->send(data,2000, "86:52:18:00:84:08", 8989);
        //}
         Delay (500000000);
    } else {
        cout << "QEMU RECEIVE | Protocol: "  << endl;
        //for (int i = 0; i<2;i++){
        sos->receive(data,2000);
        cout << "Data: " << data << endl;
        //}
        Delay (500000000);
        Delay (500000000);
        Delay (500000000);
    }

    delete sos;
}


int main()
{   
    teste_conexao();
    Delay (500000000);
}