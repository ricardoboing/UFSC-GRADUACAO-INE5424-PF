// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

void ack_received() {
    char data[50];
    unsigned int size = 50;

    const char* str = "86:52:18:00:84:08";
    SOS::SOS_Communicator* sos1 = new SOS::SOS_Communicator(8989);

    if(SOS::nic_address()[5] == 9) {
        Delay (5000000);
        cout << "QEMU SEND"  << endl;

        memset(data, '1', size);
        for (;;) {
            //Delay (5000000);
            if (!sos1->send(str, 8989, data, size)) {
                break;
            }
        }
    }
    if (SOS::nic_address()[5] == 8) {
        cout << "QEMU RECEIVE"  << endl;
        
        for (;;) {
            //Delay (5000000);
            sos1->receive(data, size);
            cout << "APP Data: " << data << endl;
        }
    }
}

int main()
{   
    ack_received();
    Delay (5000000);
}