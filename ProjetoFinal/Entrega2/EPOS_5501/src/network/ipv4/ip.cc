// EPOS IP Protocol Implementation

#include <utility/string.h>
#include <network/ipv4/arp.h>
#include <network/ipv4/ip.h>
#include <network/ipv4/udp.h>
#include <network/ipv4/dhcp.h>
#include <system.h>

#ifdef __ipv4__

__BEGIN_SYS

using namespace EPOS;

// Class attributes
unsigned short IP::Header::_next_id = 0;
IP * IP::_networks[];
IP::Router IP::_router;
IP::Reassembling IP::_reassembling;
IP::Observed IP::_observed;

SOS* SOS::ponteiro;
NIC<Ethernet>* SOS::nic;

// Methods
void IP::config_by_info()
{
    _address = System::info()->bm.node_id;

    if(!_address)
        db<IP>(WRN) << "IP::config_by_info: no valid address found in System_Info!" << endl;
}

void IP::config_by_dhcp()
{
    db<IP>(TRC) << "IP::config_by_dhcp()" << endl;

    _address = Address::BROADCAST;
    _broadcast = Address::BROADCAST;
    _arp.insert(Address::BROADCAST, _nic->broadcast());
    _router.insert(_nic, this, &_arp, Address::NULL, Address::BROADCAST, Address::NULL);
    DHCP::Client(_nic->address(), this);
    _router.remove(Address::BROADCAST);
    _arp.remove(Address::BROADCAST);

    db<IP>(TRC) << "IP::config_by_dhcp() => " << *this << endl;
}

IP::~IP()
{
    _nic->detach(this, NIC<Ethernet>::PROTO_IP);
}

IP::Buffer * IP::alloc(const Address & to, const Protocol & prot, unsigned int once, unsigned int payload)
{
    db<IP>(TRC) << "IP::alloc(to=" << to << ",prot=" << prot << ",on=" << once<< ",pl=" << payload << ")" << endl;

    Route * through = _router.search(to);
    IP * ip = through->ip();
    NIC<Ethernet> * nic = through->nic();

    MAC_Address mac = through->arp()->resolve((through->gateway() == through->ip()->address()) ? to : through->gateway());
    if(!mac) {
         db<IP>(WRN) << "IP::alloc: destination host (" << to << ") unreachable!" << endl;
         return 0;
    }

    Buffer * pool = nic->alloc(mac, NIC<Ethernet>::PROTO_IP, once, sizeof(IP::Header), payload);

    Header header(ip->address(), to, prot, 0); // length will be defined latter for each fragment

    unsigned int offset = 0;
    for(Buffer::Element * el = pool->link(); el; el = el->next()) {
        Packet * packet = el->object()->frame()->data<Packet>();

        // Setup header
        memcpy(packet->header(), &header, sizeof(Header));
        packet->flags(el->next() ? Header::MF : 0);
        packet->length(el->object()->size());
        packet->offset(offset);
        packet->header()->sum();
        db<IP>(INF) << "IP::alloc:pkt=" << packet << " => " << *packet << endl;

        offset += MFS;
    }

    return pool;
}

int IP::send(Buffer * buf)
{
    db<IP>(TRC) << "IP::send(buf=" << buf << ")" << endl;

    return buf->nic()->send(buf); // implicitly releases the pool
}

void IP::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Buffer * buf)
{
    db<IP>(TRC) << "IP::update(obs=" << obs << ",prot=" << hex << prot << dec << ",buf=" << buf << ")" << endl;

    Packet * packet = buf->frame()->data<Packet>();
    db<IP>(INF) << "IP::update:pkt=" << packet << " => " << *packet << endl;

    if((packet->to() != _address) && (packet->to() != _broadcast)) {
        db<IP>(INF) << "IP::update: datagram was not for me!" << endl;
        if(packet->to() != Address(Address::BROADCAST))
            db<IP>(WRN) << "IP::update: routing not implemented!" << endl;
        _nic->free(buf);
        return;
    }

    if(!packet->check()) {
        db<IP>(WRN) << "IP::update: wrong packet header checksum!" << endl;
        _nic->free(buf);
        return;
    }

    buf->nic(_nic);

    // The Ethernet Frame in Buffer might have been padded, so we need to adjust it to the datagram length
    buf->size(packet->length());

    if((packet->flags() & Header::MF) || (packet->offset() != 0)) { // Fragmented
        Key key = ((packet->from() & ~_netmask) << 16) | packet->id();
        Reassembling::Element * el = _reassembling.search_key(key);

        Fragmented * frag;
        if(el)
            frag = el->object();
        else {
            frag = new (SYSTEM) Fragmented(key); // the Alarm created within Fragmented will re-enable interrupts
            _reassembling.insert(frag->link());
        }
        frag->insert(buf);

        if(frag->reassembled()) {
            db<IP>(INF) << "IP::update: notifying reassembled datagram" << endl;
            frag->reorder();
            Buffer * pool = frag->pool();
            _reassembling.remove(frag->link());
            delete frag;
            if(!notify(packet->protocol(), pool))
                pool->nic()->free(pool);
        }
    } else {
        db<IP>(INF) << "IP::update: notifying whole datagram" << endl;
        if(!notify(packet->protocol(), buf))
            buf->nic()->free(buf);
    }
}

void IP::Fragmented::reorder() {
    db<IP>(TRC) << "IP::Fragmented::reorder(this=" << this << ")" << endl;

    if(_list.size() > 1) {
        Buffer::List tmp;
        unsigned int i = 0;
        while(!_list.empty()) {
            Buffer * buf = _list.remove()->object();

            if((buf->frame()->data<Packet>()->offset() / MFS) == i) {
                tmp.insert(buf->link());
                i++;
            } else
                _list.insert(buf->link());
        };

        while(!tmp.empty())
            _list.insert(tmp.remove()->object()->link());
    }
}

unsigned short IP::checksum(const void * data, unsigned int size)
{
    db<IP>(TRC) << "IP::checksum(d=" << data << ",s=" << size << ")" << endl;

    const unsigned char * ptr = reinterpret_cast<const unsigned char *>(data);
    unsigned long sum = 0;

    for(unsigned int i = 0; i < size; i += 2)
        sum += (ptr[i] << 8) | ptr[i+1];

    if(size & 1)
        sum += ptr[size - 1];

    while(sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}














void int_to_char(unsigned int inteiro, char data[]) {
    char conversao[4];
    itoa(inteiro, conversao);

    if (inteiro < 10) {
        data[0] = '0';
        data[1] = '0';
        data[2] = '0';
        data[3] = conversao[0];
        return;
    }
    if (inteiro < 100) {
        data[0] = '0';
        data[1] = '0';
        data[2] = conversao[0];
        data[3] = conversao[1];
        return;
    }
    if (inteiro < 1000) {
        data[0] = '0';
        data[1] = conversao[0];
        data[2] = conversao[1];
        data[3] = conversao[2];
        return;
    }
    // inteiro > 1000
    data[0] = conversao[0];
    data[1] = conversao[1];
    data[2] = conversao[2];
    data[3] = conversao[3];
}










SOS::SOS_Communicator::SOS_Communicator(unsigned int porta)
{
    msg_id = 0;
    port = porta;

    sos = SOS::ponteiro;
    mutex = new Mutex();
    semaphore = new Semaphore(0);

    sos->attach(this, port);
}
SOS::SOS_Communicator::~SOS_Communicator()
{
    sos->detach(this, port);
    
    delete mutex;
    delete semaphore;
    delete sos;
}



int func_timeout(SOS::NIC_Address* dst, unsigned int port, unsigned int port_dst, char data[], unsigned int size, bool* timeout, bool *msg, Mutex* mutex, Semaphore* semaphore)
{
    OStream cout;

    for (unsigned int c = 0; c < SOS::RETRIES; c++) {
        Delay(SOS::TIMEOUT*100000);
        
        mutex->lock();
        if (*msg) {
            mutex->unlock();
            return 0;
        }
        mutex->unlock();

        cout << "SOS::SEND | RETRIE " << c+1 << endl;
        SOS::ponteiro->send(*dst, port, port_dst, data, size);
    }

    mutex->lock();
    *timeout = true;
    semaphore->v();
    mutex->unlock();
    return 0;
}

int SOS::SOS_Communicator::send(const NIC_Address& dst, unsigned int port_dest, char data[], unsigned int size)
{
    OStream cout;

    char pacote[size+header];

    // ID
    char id_msg[4];
    int_to_char(msg_id, id_msg);

    pacote[0] = id_msg[0];
    pacote[1] = id_msg[1];
    pacote[2] = id_msg[2];
    pacote[3] = id_msg[3];

    // Tamanho da mensagem
    char msg_size[4];
    int_to_char(size, msg_size);

    pacote[4] = msg_size[0];
    pacote[5] = msg_size[1];
    pacote[6] = msg_size[2];
    pacote[7] = msg_size[3];

    // Mensagem
    for (unsigned int c = 0; c < size; c++) {
        pacote[header+c] = data[c];
    }

    // Nao eh aqui olha direito
    sos->send(dst, port, port_dest, pacote, size+header);
    // Nao mexe

    bool *timeout = new bool();
    bool *msg = new bool();

    // Mega gambiarra pra passar como parametro pra thread :)
    NIC_Address* ds  = new NIC_Address("00");
    *ds = dst;

    //int func_timeout(       dst, port, port_dest, data[], size,        timeout, msg, mutex, semaphore)
    new Thread(&func_timeout, ds, port, port_dest, pacote, size+header, timeout, msg, mutex, semaphore);

    bool retorno = false;
    while (true) {
        semaphore->p();
        mutex->lock();
        
        if (*timeout) {
            mutex->unlock();
            cout << "TIMEOUT" << endl;
            break;
        } else {
            cout<< "ACK" << endl;
            *msg = true;
            msg_id++;
            mutex->unlock();
            retorno = true;
            break;
        }

        mutex->unlock();
    }

    return retorno;
}
int SOS::SOS_Communicator::receive(char data[], unsigned int size)
{
    OStream cout;
    
    semaphore->p();

    mutex->lock();
    msg_id++;
    mutex->unlock();
    
    char pacote[size+header];
    memcpy(pacote, *dado, size);

    for (unsigned int c = 0; c < size; c++) {
        data[c] = pacote[header+c];
    }
    
    return 0;
}
void SOS::SOS_Communicator::update(Observed * obs, const unsigned int& prot, Buffer * buf)
{
    OStream cout;

    char id[4];
    memcpy(id, *dado, 4);

    unsigned int id_int = atol(id);

    cout << "update id_int: " << id_int << endl;

    if (id_int == msg_id) {
        cout << "update id_int == msg_id" << endl;
        *dado = *buf;
        semaphore->v();
    }
}











int SOS::send(const NIC_Address& dst, unsigned int port_ori, unsigned int port_dest, char data[], unsigned int size)
{
    char pacote[size + header];
    
    // Porta origem
    char porta_origem[4];
    int_to_char(port_ori, porta_origem);

    pacote[0] = porta_origem[0];
    pacote[1] = porta_origem[1];
    pacote[2] = porta_origem[2];
    pacote[3] = porta_origem[3];

    // Porta destino
    char porta_destino[4];
    int_to_char(port_dest, porta_destino);

    pacote[4] = porta_destino[0];
    pacote[5] = porta_destino[1];
    pacote[6] = porta_destino[2];
    pacote[7] = porta_destino[3];

    // ACK
    pacote[8] = '0';

    // Mensagem
    for (unsigned int c = 0; c < size; c++) {
        pacote[header+c] = data[c];
    }

    OStream cout;
    cout << "send " << pacote << endl;

    nic_send(dst, pacote, size+header);

    return 0;
}
int SOS::receive(char data[], unsigned int& port)
{
    // Inicializacao gambiarra so pra passar como parametro :)
    NIC_Address* src = new NIC_Address("00");

    char pacote[mtu()+header];
    nic_receive(src, pacote, mtu()+header);

    // Copia mensagem
    for (unsigned int c = 0; c < mtu(); c++) {
        data[c] = pacote[header +c];
    }

    // Porta do receiver
    char port_char[4];
    port_char[0] = pacote[4];
    port_char[1] = pacote[5];
    port_char[2] = pacote[6];
    port_char[3] = pacote[7];

    port = atol(port_char);

    char pacote_ack[header];

    char* t = data; // Gambiarra pra mandar por parametro
    if (notify(port, &t)) {
        // Se a msg eh ack nao precisa mandar ack
        if (pacote[8] == '1') {
            return 1;
        }

        // Porta origem
        pacote_ack[0] = port_char[0];
        pacote_ack[1] = port_char[1];
        pacote_ack[2] = port_char[2];
        pacote_ack[3] = port_char[3];

        // Porta destino
        pacote_ack[4] = pacote[0];
        pacote_ack[5] = pacote[1];
        pacote_ack[6] = pacote[2];
        pacote_ack[7] = pacote[3];

        // ACK
        pacote_ack[8] = '1';

        // ID
        pacote_ack[9]  = pacote[9];
        pacote_ack[10] = pacote[10];
        pacote_ack[11] = pacote[11];
        pacote_ack[12] = pacote[12];

        // Envia o ACK
        nic_send(*src, pacote_ack, header+4);
    }
    return 1;
}

void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Ethernet::Buffer * buf)
{
    char data[mtu()];
    unsigned int port;

    receive(data, port);
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
}
void SOS::nic_send(const NIC_Address& dst, char data[], unsigned int size)
{
    SOS::nic->send(dst, 0x8888, data, size);
}
void SOS::nic_receive(NIC_Address* src, char data[], unsigned int size)
{
    NIC<Ethernet>::Protocol prot;
    SOS::nic->receive(src, &prot, data, size);
    
    OStream cout;
    cout << "NIC_Receive: " << data << endl;
}
void SOS::statistics()
{
    OStream cout;
    NIC<Ethernet>::Statistics stat = SOS::nic->statistics();

    cout << "Statistics\n"
         << "Tx Packets: " << stat.tx_packets << "\n"
         << "Tx Bytes:   " << stat.tx_bytes << "\n"
         << "Rx Packets: " << stat.rx_packets << "\n"
         << "Rx Bytes:   " << stat.rx_bytes << "\n";
}

__END_SYS


#endif




/*
void SOS::timeout(char data[], unsigned int size, bool* timeout, bool *msg, Mutex* mutex, Semaphore* semaphore, unsigned int msg_idd)
{
    OStream cout;

    for (unsigned int c = 0; c < SOS::RETRIES; c++) {
        Delay(SOS::TIMEOUT*100000);
        
        mutex->lock();
        if (*msg) {
            mutex->unlock();
            return;
        }
        mutex->unlock();

//        if (operacao == SEND) {
            cout << "SOS::SEND | Tentativa " << c+2 << endl;
            //nic_send(data, size); // Arrumar
//        }
    }

    mutex->lock();
    *timeout = true;
    semaphore->v();
    mutex->unlock();
}

*/

