// EPOS SOS Protocol Implementation

#include <utility/string.h>
#include <network/sos/sos.h>
#include <system.h>
#include <time.h>

#ifdef __sos__

__BEGIN_SYS

using namespace EPOS;

SOS* SOS::ponteiro;
NIC<Ethernet>* SOS::nic;
SOS::Tick SOS::tick1;
SOS::Tick SOS::tick2;
SOS::Tick SOS::tick3;
SOS::Tick SOS::ultimo_elapsed;

int SOS::SOS_Communicator::send(const char* address, unsigned int port_dest, char data[], unsigned int size)
{
    OStream cout;

    SOS::NIC_Address addr(address);

    Pacote pacote;
    pacote.id = msg_id;
    pacote.size = size;
    pacote.port_source = port;
    pacote.port_destination = port_dest;
    memcpy(pacote.data, data, size);

    sos->send(addr, &pacote);

    Semaphore_Handler handler(semaphore);
    for (unsigned int c = 0; c < SOS::RETRIES; c++) {
        Alarm alarm(SOS::TIMEOUT, &handler);
        
        semaphore->p();

        mutex->lock();
        if (msg_id != pacote.id) {
            mutex->unlock();
            return 1;
        }
        mutex->unlock();

        cout << "RETRIE " << c << " | msg_id: " << msg_id << endl;
        sos->send(addr, &pacote);
    }

    return 0;
}
int SOS::SOS_Communicator::receive(char data[], unsigned int size)
{
    semaphore->p();

    Doubly_Linked<Pacote>* link = pacotes->remove_head();
    Pacote* pacote = link->object();

    memcpy(data, pacote->data, size);
    
    delete link;
    delete pacote;

    return 0;
}
void SOS::SOS_Communicator::update(Observed * obs, const unsigned int& prot, Buffer * buf)
{
    OStream cout;
    
    Pacote* pacote = new Pacote();
    memcpy(pacote, buf->frame()->data<void>(), sizeof(Pacote));

    if (pacote->type == MSG_TYPE_ACK) { // SEND
        if (pacote->id == msg_id) {
            mutex->lock();
            msg_id++;
            mutex->unlock();
            semaphore->v();
        }

        //cout << "ACK " << pacote->id << " ";
    } else { // RECEIVE
        NIC_Address address = buf->frame()->src();

        Cliente* cliente = client(address);

        if (pacote->id <= cliente->id) {
            Pacote pacote_ack;
            pacote_ack.port_destination = pacote->port_source;
            pacote_ack.port_source = pacote->port_destination;
            pacote_ack.id = pacote->id;
            pacote_ack.size = pacote->size;
            pacote_ack.type = MSG_TYPE_ACK;

            sos->send(address, &pacote_ack);
        }

        //cout << "Pacote.id: " << pacote->id << " | Cliente->id: " << cliente->id << endl;

        if (pacote->id == cliente->id) {
            cliente->id++;

            pacotes->insert( new Doubly_Linked<Pacote>(pacote) );
            semaphore->v();
        }
    }

    nic->free(buf);
}
SOS::SOS_Communicator::SOS_Communicator(unsigned int porta)
{
    msg_id = 0;
    port = porta;

    sos = SOS::ponteiro;
    mutex = new Mutex();
    semaphore = new Semaphore(0);

    clientes = new List<Cliente>();
    pacotes = new List<Pacote>();

    sos->attach(this, port);
}
SOS::SOS_Communicator::~SOS_Communicator()
{
    sos->detach(this, port);
    
    delete mutex;
    delete semaphore;
}
SOS::SOS_Communicator::Cliente* SOS::SOS_Communicator::client(NIC_Address& address) {
    Cliente* cliente = 0;

    if (!clientes->empty()) {
        for (Doubly_Linked<Cliente>* next = clientes->head(); next != 0; next = next->next()) {
            if (next->object()->address == address) {
                cliente = next->object();
                break;
            }
        }
    }

    if (cliente == 0) {
        cliente = new Cliente();
        memcpy(&(cliente->address), &address, sizeof(NIC_Address));

        Doubly_Linked<Cliente>* link = new Doubly_Linked<Cliente>(cliente);

        clientes->insert(link);
    } 

    return cliente;
}
void SOS::send(const NIC_Address& address, Pacote* pacote)
{   
    pacote->elapsed = Alarm::elapsed();
    
    SOS::nic->send(address, protocol, pacote, sizeof(Pacote));

    if (tick2 != 0 && tick3 == 0) {
        tick3 = pacote->elapsed;
    }
}
void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Ethernet::Buffer * buf)
{
    OStream cout;

    Pacote pacote;
    memcpy(&pacote, buf->frame()->data<void>(), sizeof(Pacote));

    NIC_Address address = buf->frame()->src();

    if (address[5] == 8) { // Endereco do servidor
        if (pacote.elapsed - ultimo_elapsed < 2000000) { // Intervalo aceitavel
            if (tick2 == 0) {
                tick2 = Alarm::elapsed();
                tick1 = pacote.elapsed;
            } else if (tick3 != 0) {
                Tick tick4 = pacote.elapsed;

                cout << "Clock_velho: " << Alarm::elapsed();

                Tick pd = ((tick2 - tick1) + (tick4 - tick3)) / 2;
                Tick offset = (tick2 - tick1) - pd;

                Alarm::_elapsed = Alarm::elapsed() - offset;
                
                cout << " | Clock_novo: " << Alarm::elapsed() << endl;

                tick1 = 0;
                tick2 = 0;
                tick3 = 0;
            }
        } else {
            tick1 = 0;
            tick2 = 0;
            tick3 = 0;
        }

        ultimo_elapsed = pacote.elapsed;
    } else {
        cout << "Clock: " << Alarm::elapsed() << endl;
    }

    if (!notify(pacote.port_destination, buf)) {
        nic->free(buf);
    }
}
SOS::SOS()
{
    SOS::nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    SOS::nic->attach(this, protocol);

    _observed = new Observed();
}
SOS::~SOS()
{
    SOS::nic->detach(this, protocol);
    delete _observed;
}

__END_SYS

#endif
