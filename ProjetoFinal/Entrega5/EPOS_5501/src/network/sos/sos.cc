// EPOS SOS Protocol Implementation

#include <utility/string.h>
#include <network/sos/sos.h>
#include <system.h>
#include <time.h>
#include <utility/geometry.h>
#include <utility/math.h>

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
    //cout<< gpgga << " serial "<<endl;
    //"$GPGGA,134658.00,0,N,0.785398,E,2,09,1.0,-6377995.6,M,-16.27,M,08,AAAA*60"
    
    int position = 0;
    float latitude = 0;
    float longitude = 0;
    float altitude = 0;
    char* temp_str = new char[50];
    int temp_position = 0;
    
    for(unsigned int i = 0; i<strlen(gpgga); i++){
        if(gpgga[i] == ','){
            position++;
            continue;
        }

        if(position == 2){
            temp_str[temp_position] = gpgga[i];
            temp_position++;
            

        } else if(position == 3){
            latitude = str_to_float(temp_str);
            if(gpgga[i] == 'S'){
                latitude *= -1;
            }
            temp_str = new char[50];
            temp_position = 0;

        } else if(position == 4){
                temp_str[temp_position] = gpgga[i];
                temp_position++;
        
        } else if(position == 5){
            longitude = str_to_float(temp_str);
            if(gpgga[i] == 'W'){
                longitude *= -1;
            }
            temp_str = new char[50];
            temp_position = 0;

        } else if(position == 9){
            temp_str[temp_position] = gpgga[i];
            temp_position++; 

        } else if(position == 10){
            if(gpgga[i] == 'M'){
                altitude = str_to_float(temp_str);
            }
            break;
        }
    }
    cout << "latitude " << latitude << endl 
    << "longitude "<< longitude <<endl 
    << "altitude "<< altitude <<endl;

    //float n = a / sqrt(1 - e2 * sin(latitude) * sin(latitude));
    //cout << sin(latitude)<< "sin"<<endl;
    //float x = (n + altitude) * cos(latitude) * cos(longitude);
    //float y = (n + altitude) * cos(latitude) * sin(longitude);
    //float z = (n * (1 - e2) + altitude) * sin(latitude);

    //cout << "(" <<x<<","<<y<<","<<z<<")"<< endl;
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
    uart->put(count + 48);
    int c = uart->get();
    int len=0;
    while(c != eof ){
        init[len] = (char) c;
        len++;
        c = uart->get();
    }
    return msg;
}

float GPS_Driver::str_to_float(char* p){
    // here i took another two   variables for counting the number of digits in mantissa
  int i, num = 0, num2 = 0, pnt_seen = 0, x = 0, y = 1 , signal = 1,init = 0; 
  float f2, f3;
  if(p[0] == '-'){
    signal = -1;
    init = 1;
  }
  for (i = 0; p[i]; i++)
    if (p[i] == '.') {
      pnt_seen = i;
      break;
    }
  for (i = init; p[i]; i++) {
    if (i < pnt_seen) num = num * 10 + (p[i] - 48);
    else if (i == pnt_seen) continue;
    else {
      num2 = num2 * 10 + (p[i] - 48);
      ++x;
    }
  }
  // it takes 10 if it has 1 digit ,100 if it has 2 digits in mantissa
  for (i = 1; i <= x; i++) 
    y = y * 10;
  f2 = num2 / (float) y;
  f3 = num + f2;
  return f3*signal;
}

float GPS_Driver::factorial(int x)  
{
    float fact = 1;
    for(; x >= 1 ; x--)
    {
        fact = x * fact;
    }
    return fact;
}

float GPS_Driver::sin(float radians)  //value of sine by Taylors series
{
   float a,b,c;
   float result = 0;
   for(int y=0 ; y < 9 ; y++)
   {
      a=  pow(-1,y);
      b=  pow(radians,(2*y)+1);
      c=  factorial((2*y)+1);
      result = result+ (a*b)/c;
   }
   return result;
}


int SOS::SOS_Communicator::send(const char* address, unsigned int port_dest, char data[], unsigned int size)
{
    OStream cout;

    SOS::NIC_Address addr(address);

    Packet* pacote = new Packet();
    pacote->id = msg_id;
    pacote->size = size;
    pacote->port_source = port;
    pacote->port_destination = port_dest;
    memcpy(pacote->data, data, size);

    packet_not_ack->insert( new Doubly_Linked<Packet>(pacote) );

    sos->send(addr, pacote);

    Semaphore_Handler handler(semaphore_send);
    for (unsigned int c = 0; c < SOS::RETRIES; c++) {
        Alarm alarm(SOS::TIMEOUT, &handler);
        
        semaphore_send->p();

        mutex_send->lock();
        if (msg_id != pacote->id) {
            cout << "ACK!" << endl;
            mutex_send->unlock();
            return 1;
        }
        mutex_send->unlock();

        cout << "RETRIE " << c << " | msg_id: " << msg_id << endl;
        sos->send(addr, pacote);
    }

    return 0;
}
int SOS::SOS_Communicator::receive(char data[], unsigned int size)
{
    semaphore_receive->p();

    Doubly_Linked<Packet>* link = packet_receive->remove_head();
    Packet* pacote = link->object();

    memcpy(data, pacote->data, size);
    
    delete link;
    delete pacote;

    return 0;
}
void SOS::SOS_Communicator::update(Observed * obs, const unsigned int& prot, Buffer * buf)
{
    Packet* pacote = new Packet();
    memcpy(pacote, buf->frame()->data<void>(), sizeof(Packet));

    if (pacote->type == MSG_TYPE_ACK) { // SEND
        if (pacote->id == msg_id) {
            mutex_send->lock();
            msg_id++;
            mutex_send->unlock();
            semaphore_send->v();

            if (!packet_not_ack->empty()) {
                for (Doubly_Linked<Packet>* next = packet_not_ack->head(); next != 0; next = next->next()) {
                    if (next->object()->id == pacote->id) {
                        packet_not_ack->remove(next);
                        delete next;
                        break;
                    }
                }
            }
        }
    } else { // RECEIVE
        NIC_Address address = buf->frame()->src();

        Client* cliente = client(address);

        if (pacote->id <= cliente->id) {
            Packet pacote_ack;
            pacote_ack.port_destination = pacote->port_source;
            pacote_ack.port_source = pacote->port_destination;
            pacote_ack.id = pacote->id;
            pacote_ack.size = pacote->size;
            pacote_ack.type = MSG_TYPE_ACK;

            sos->send(address, &pacote_ack);
        }

        if (pacote->id == cliente->id) {
            cliente->id++;

            packet_receive->insert( new Doubly_Linked<Packet>(pacote) );
            semaphore_receive->v();
        }
    }

    nic->free(buf);
}
SOS::SOS_Communicator::SOS_Communicator(unsigned int porta)
{
    msg_id = 0;
    port = porta;

    sos = SOS::ponteiro;
    mutex_send = new Mutex();
    semaphore_send = new Semaphore(0);
    semaphore_receive = new Semaphore(0);

    clients = new List<Client>();
    packet_receive = new List<Packet>();
    packet_not_ack = new List<Packet>();

    sos->attach(this, port);
}
SOS::SOS_Communicator::~SOS_Communicator()
{
    sos->detach(this, port);
    
    delete mutex_send;
    delete semaphore_send;
    delete semaphore_receive;
}
SOS::SOS_Communicator::Client* SOS::SOS_Communicator::client(NIC_Address& address) {
    Client* cliente = 0;

    if (!clients->empty()) {
        for (Doubly_Linked<Client>* next = clients->head(); next != 0; next = next->next()) {
            if (next->object()->address == address) {
                cliente = next->object();
                break;
            }
        }
    }

    if (cliente == 0) {
        cliente = new Client();
        memcpy(&(cliente->address), &address, sizeof(NIC_Address));

        Doubly_Linked<Client>* link = new Doubly_Linked<Client>(cliente);

        clients->insert(link);
    } 

    return cliente;
}
void SOS::send(const NIC_Address& address, Packet* pacote)
{   
    pacote->elapsed = Alarm::elapsed();

    if (nic_address()[5] == 8) {
        position = gps->get_coord();
        valid_position = true;
    }

    pacote->coordinates = position;
    pacote->valid_position = valid_position;

    SOS::nic->send(address, protocol, pacote, sizeof(Packet));

    if (tick2 != 0 && tick3 == 0) {
        tick3 = pacote->elapsed;
    }
}
void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Ethernet::Buffer * buf)
{
    Packet pacote;
    memcpy(&pacote, buf->frame()->data<void>(), sizeof(Packet));

    NIC_Address address = buf->frame()->src();
    
    if (address[5] == 8) { // Endereco do servidor
        if (pacote.elapsed - ultimo_elapsed < 2000000) { // Intervalo aceitavel
            if (tick2 == 0) {
                tick2 = Alarm::elapsed();
                tick1 = pacote.elapsed;
            } else if (tick3 != 0) {
                Tick tick4 = pacote.elapsed;

                Tick pd = ((tick2 - tick1) + (tick4 - tick3)) / 2;
                Tick offset = (tick2 - tick1) - pd;

                Alarm::_elapsed = Alarm::elapsed() - offset;
                
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

        if(!anchor1.valid_position) {
            anchor1.position = pacote.coordinates;
            anchor1.valid_position = true;
            anchor1.distance = spected_position - pacote.coordinates;
        } else if(!anchor2.valid_position ) {
            if(anchor1.position != pacote.coordinates) {
                anchor2.position = pacote.coordinates;
                anchor2.valid_position = true;
                anchor2.distance = spected_position - pacote.coordinates;
            }
        } else {
            if(anchor1.position != pacote.coordinates && anchor2.position != pacote.coordinates) {
                anchor3.position = pacote.coordinates;
                anchor3.valid_position = true;
                anchor3.distance = spected_position - pacote.coordinates;

                position = position.trilaterate(anchor1.position, anchor1.distance, anchor2.position, anchor2.distance, anchor3.position, anchor3.distance);
                valid_position = true;
                anchor1.valid_position = false;
                anchor2.valid_position = false;
                anchor3.valid_position = false;
            }
        }
    }

    if (pacote.type == MSG_TYPE_GPS) {
        if (nic_address()[5] == 8 && !pacote.valid_position) {
            send(address, &pacote);
        }
        nic->free(buf);
    } else if (!notify(pacote.port_destination, buf)) {
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
