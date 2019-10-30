// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

void ack_received() {
    cout << "Teste de conexao 1" << endl;
    char data[50];

    const char* str = "86:52:18:00:84:08";
    SOS::NIC_Address addr(str);
    SOS::SOS_Communicator* sos1 = new SOS::SOS_Communicator(8989);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND"  << endl;

        for (;;) {//int i = 0; i < 400; i++) {
            memset(data, '1', 50);
            data[50 - 1] = '\n';

            if (!sos1->send(addr, 8989, data, 50)) {
                break;
            }
        }
    } else {
        cout << "QEMU RECEIVE"  << endl;
        
        for (;;) {//(int i = 0; i < 400; i++) {
            sos1->receive(data, 50);
            cout << "APP Data: " << data << endl;
        }
    }
}

int main()
{   
    ack_received();
}