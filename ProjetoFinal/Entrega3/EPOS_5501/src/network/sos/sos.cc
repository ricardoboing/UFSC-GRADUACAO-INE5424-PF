// EPOS SOS Protocol Implementation

#include <utility/string.h>
#include <network/sos/sos.h>
#include <system.h>
#include <time.h>
#include <utility/geometry.h>

#ifdef __sos__

__BEGIN_SYS

using namespace EPOS;

SOS* SOS::ponteiro;
NIC<Ethernet>* SOS::nic;
SOS::Tick SOS::tick1;
SOS::Tick SOS::tick2;
SOS::Tick SOS::tick3;
SOS::Tick SOS::ultimo_elapsed;

GPS_Driver::GPS_Driver(){
    uart = new UART(1, 115200, 8, 0, 1);
    uart->loopback(false);
    count = 0;
}
GPS_Driver::~GPS_Driver(){
    delete uart;

}

Point<int,3> GPS_Driver::get_coord(){
    
    char* gpgga = get_data_from_serial();
    OStream cout;
    cout<< gpgga << " serial "<<endl;
    int a = 100, b = 100, c = 0;
    if(count==1){
        a=0;
        b=0;
    }
    if(count==2){
        a=0;
        count = -1;
    }
    count++;
    return Point<int,3>(a,b,c);

}

char* GPS_Driver::get_data_from_serial(){
    OStream cout;
    char init[1000];
    char* msg = &init[0];
    uart->put(48);
    int c = uart->get();
    int len=0;
    while(c != eof ){
        init[len] = (char) c;
        len++;
        c = uart->get();
    }
    return msg;
}


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
    OStream cout;
    //cout << position <<" pos init"<<endl;

    if (nic_address()[5] == 8) {
        position = gps->get_coord();
        valid_position = true;
    }
    unsigned int port_temp = pacote->port_destination;
    while(!valid_position){
        pacote->port_destination = 0;
        SOS::NIC_Address addr("86:52:18:00:84:08");
        SOS::nic->send(addr, protocol, pacote, sizeof(Pacote));
        Delay (5000);
        
    }
    pacote->port_destination = port_temp;
    pacote->coordinates = position;
    pacote->valid_position = valid_position;

    cout << pacote->coordinates << " position package snd"<<endl;
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
    if (nic_address()[5] == 8 && !pacote.valid_position) {
        send( address, &pacote);
    }
    cout << pacote.coordinates << " position package rcv"<<endl;
    if (address[5] == 8) { // Endereco do servidor
        if (pacote.elapsed - ultimo_elapsed < 2000000) { // Intervalo aceitavel
            if (tick2 == 0) {
                tick2 = Alarm::elapsed();
                tick1 = pacote.elapsed;
            } else if (tick3 != 0) {
                Tick tick4 = pacote.elapsed;

                //cout << "Clock_velho: " << Alarm::elapsed();

                Tick pd = ((tick2 - tick1) + (tick4 - tick3)) / 2;
                Tick offset = (tick2 - tick1) - pd;

                Alarm::_elapsed = Alarm::elapsed() - offset;
                
                //cout << " | Clock_novo: " << Alarm::elapsed() << endl;

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

        if(!anchor1.valid_position){
            
            anchor1.position = pacote.coordinates;
            anchor1.valid_position = true;
            anchor1.distance = spected_position - pacote.coordinates;

        }else if(!anchor2.valid_position ){
            if(anchor1.position != pacote.coordinates){
                anchor2.position = pacote.coordinates;
                anchor2.valid_position = true;
                anchor2.distance = spected_position - pacote.coordinates;
            }

        }else{
            if(anchor1.position != pacote.coordinates && anchor2.position != pacote.coordinates){
                anchor3.position = pacote.coordinates;
                anchor3.valid_position = true;
                anchor3.distance = spected_position - pacote.coordinates;

                position = position.trilaterate(anchor1.position, anchor1.distance, anchor2.position, anchor2.distance, anchor3.position, anchor3.distance);
                valid_position = true;
                anchor1.valid_position = false;
                anchor2.valid_position = false;
                anchor3.valid_position = false;
                cout <<"esperada " << spected_position << ", position "<< position << endl;
            }

        }
           

    } else {
        //cout << "Clock: " << Alarm::elapsed() << endl;
    }

    //cout << pacote.coordinates<< endl;

    if (!notify(pacote.port_destination, buf)) {
        nic->free(buf);
    }
}
SOS::SOS()
{
    SOS::nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    SOS::nic->attach(this, protocol);
    valid_position = false;
    if (nic_address()[5] == 8) {
        gps = new GPS_Driver();
    }
    spected_position = Point<int,3>(100,90,0);
    _observed = new Observed();
}
SOS::~SOS()
{
    SOS::nic->detach(this, protocol);
    delete _observed;
}




        


__END_SYS

#endif
