// EPOS NIC Test Programs

#include <time.h>
#include <communicator.h>

using namespace EPOS;
OStream cout;

/* TESTE DE DMA */
unsigned int fibbonacci(unsigned int n) { // Sugerido pelo professor
    if (n == 1 || n == 2) {
        return 1;
    } else {
        return fibbonacci(n-1) + fibbonacci(n-2);
    }
}
int teste_dma_cpu_bound() {
    fibbonacci(30);
}
int teste_dma_nic() {
    char data[1000];

    SOS* sos = new SOS(SOS::PROTOCOL_SOS);
    
    if(SOS::nic_address()[5] % 2) {
        Delay (5000000);
        memset(data, '1', 1000);
        data[1000 - 1] = '\n';
            
        sos->send(data);
    } else {
        sos->rcv(data);
    }
}
void teste_dma_nic_cpu_bound() {
    Thread *cpu, *dma;

    cpu = new Thread(&teste_dma_cpu_bound);
    dma = new Thread(&teste_dma_nic);
    
    cpu->join();
    dma->join();

    delete cpu;
    delete dma;
}

void teste_dma() {
    unsigned int time_nic, time_cpu_bound, time_nic_cpu_bound;
    Chronometer cronometro;
    
    // NIC
    cronometro.start();
    teste_dma_nic();
    cronometro.stop();
    
    time_nic = cronometro.read() / 1000;

    // CPU BOUND
    cronometro.reset();
    cronometro.start();
    teste_dma_cpu_bound();
    cronometro.stop();
    
    time_cpu_bound = cronometro.read() / 1000;

    // NIC & CPU BOUND
    cronometro.reset();
    cronometro.start();
    teste_dma_nic_cpu_bound();
    cronometro.stop();
    
    time_nic_cpu_bound = cronometro.read() / 1000;

    cout << "NIC:             " << time_nic << " segundos" << endl;
    cout << "CPU BOUND:       " << time_cpu_bound << " segundos" << endl;
    cout << "NIC + CPU BOUND: " << time_nic_cpu_bound << " segundos" << endl;
}

int main()
{   
    teste_dma();
}