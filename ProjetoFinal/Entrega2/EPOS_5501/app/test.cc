// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

void teste_protocolo() {
    cout << "Teste de protocolo" << endl;
    char data[SOS::mtu()];

    SOS* sos = new SOS(8989);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND" << endl;
        
        for (int c = 0; c < 20; c++) {
            memset(data, '2', SOS::mtu());
            
            if ( !sos->send(data, sizeof(data)) ) {
                cout << "ERROW" << endl;
                break;
            }
        }
        SOS::statistics();
    } else {
        Delay (5000000);
        cout << "QEMU RECEIVE" << endl;
        for (int i = 0; i < 20; i++) {
            memset(data, '0', SOS::mtu());
            if ( !sos->receive(data, SOS::mtu()) ) {
                cout << "ERROW" << endl;
                break;
            }
            cout << "Data: " << data << " | Size: " << sizeof(data) << endl;
        }
        SOS::statistics();
    }
}


int main()
{   
    teste_protocolo();
    return 0;
}




/*
void teste_protocolo() {
    cout << "Teste de protocolo" << endl;
    char data[1];
    char ack[1];
    SOS* sos = new SOS(8989);

    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        cout << "QEMU SEND" << endl;

        for (int c = 0; c < 20; c++) {
            memset(data, '0', 1);
            
            cout << " ---------- " << endl;
            if ( !sos->send(data) ) {
                cout << "ERROW" << endl;
                break;
            }
            sos->receive(data, 1);
            cout << "Data: " << data << endl;
        }
        SOS::statistics();
    } else {
        cout << "QEMU RECEIVE" << endl;
        for (int i = 0; i < 20; i++) {
            memset(ack, '1', 1);
            cout << "RCV: " << i << endl;
            sos->receive(data, 1);
            cout << "Data: " << data << endl;
            sos->send(ack);
        }
    }
}

*/





















/*
Semaphore* semaphore;
Mutex* mutex;

int function_a() {
    Delay(5000000);
    semaphore->p();

    for (int c = 0; c < 1; c++) {
        
        
        //mutex->lock();
        cout << "function_a" << endl;
    }
}
int function_b(Thread *a) {
    Delay(5000000);
    semaphore->p();
    
    for (int c = 0; c < 1; c++) {
        
        //mutex->unlock();
        cout << "function_b" << endl;
    }
}
int function_c() {
    semaphore->v();
    semaphore->v();
    cout << "function_c" << endl;
}

void function_d() {
    //mutex = new Mutex();
    semaphore = new Semaphore(0);

    Thread* a = new Thread(&function_a);
    Thread* b = new Thread(&function_b, a);
    Thread* c = new Thread(&function_c);

    a->join();
    b->join();
    c->join();
}

*/