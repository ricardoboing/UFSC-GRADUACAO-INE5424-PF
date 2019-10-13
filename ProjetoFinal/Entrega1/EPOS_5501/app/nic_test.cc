// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;


int teste_conexao(){
    SOS* sos = new SOS(0x8888);
    char data[1000];
    if(SOS::mac()[5] % 2) { // sender
        Delay (5000000);
        cout << "SENDER" << endl;
            memset(data, '1', 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
    } else { // receiver
        cout << "RECIEVER" << endl;
        sos->rcv(data);
        cout << "Data: " << data << endl;
    }
    delete sos;
    return 0;

}


void teste_protocolo() {
    cout << "NIC Test protocol" << endl;
    char data[1000];

    SOS* sos = new SOS(0x8888);

    if(SOS::mac()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SENDER 0x8888" << endl;
        
        memset(data, '2', 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
        
    } else {
        cout << "QEMU RECEIVER 0x8888"  << endl;
        sos->rcv(data);
        cout << "Data: " << data << endl;
    }

    sos = new SOS(0x888);

    if(SOS::mac()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SENDER 0x888" << endl;
        
        memset(data, '3', 1000);
            data[1000 - 1] = '\n';
            
            sos->send(data);
        
    } else {
        cout << "QEMU RECEIVER 0x888"  << endl;
        sos->rcv(data);
        cout << "Data: " << data << endl;
    }
    delete sos;
}




int main()
{   
    teste_conexao();
    teste_protocolo();
    Delay (500000000);
}
