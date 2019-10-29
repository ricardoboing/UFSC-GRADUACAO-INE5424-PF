// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

int testu() {
    char data[50];

    const char* str = "86:52:18:00:84:08";
    SOS::NIC_Address addr(str);
    SOS::SOS_Communicator* sos = new SOS::SOS_Communicator(8989);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND"  << endl;

        for (int i = 0; i < 20; i++) {
            memset(data, '1', 50);
            data[50 - 1] = '\n';

            sos->send(addr, 8989, data, 50);
            Delay(500000);
        }
    } else {
        cout << "QEMU RECEIVE"  << endl;
        
        for (int i = 0; i < 20; i++) {
            sos->receive(data, 50);
            cout << "APP Data: " << data << endl;
        }
    }

    return 0;
}
int testa() {
    char data[50];

    const char* str = "86:52:18:00:84:08";
    SOS::NIC_Address addr(str);
    SOS::SOS_Communicator* sos = new SOS::SOS_Communicator(8988);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND"  << endl;

        for (int i = 0; i < 20; i++) {
            memset(data, '1', 50);
            data[50 - 1] = '\n';

            sos->send(addr, 8988, data, 50);
            Delay(500000);
        }
    } else {
        cout << "QEMU RECEIVE"  << endl;
        
        for (int i = 0; i < 20; i++) {
            sos->receive(data, 50);
            cout << "APP Data: " << data << endl;
        }
    }

    return 0;
}
void testy() {
    cout << "Teste de conexao 1" << endl;
    Thread* th1 = new Thread(&testu);
    Thread* th2 = new Thread(&testa);

    th1->join();
    th2->join();
}

/* TESTE DE CONEXAO */
void teste_conexao() {
    cout << "Teste de conexao 1" << endl;
    char data[50];

    const char* str = "86:52:18:00:84:08";
    SOS::NIC_Address addr(str);
    SOS::SOS_Communicator* sos1 = new SOS::SOS_Communicator(8988);
    SOS::SOS_Communicator* sos2 = new SOS::SOS_Communicator(8989);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND"  << endl;

        for (int i = 0; i < 20; i++) {
            memset(data, '1', 50);
            data[50 - 1] = '\n';

            sos1->send(addr, 8989, data, 50);
            sos2->send(addr, 8988, data, 50);
            Delay(500000);
        }
    } else {
        cout << "QEMU RECEIVE"  << endl;
        
        for (int i = 0; i < 20; i++) {
            sos1->receive(data, 50);
            sos2->receive(data, 50);
            cout << "APP Data: " << data << endl;
        }
    }
}

int main()
{   
    //testy();//e_conexao();
    teste_conexao();
}