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













int SOS::SOS_Communicator::send(const char* address, unsigned int port_dest, char data[], unsigned int size)
{
    OStream cout;

    SOS::NIC_Address addr(address);

    Pacote pacote;
    pacote.id = msg_id;
    pacote.size = size;
    pacote.type = 0; // Nao eh ack
    pacote.port_source = port;
    pacote.port_destination = port_dest;
    memcpy(pacote.data, data, size);

    sos->send(addr, &pacote, size);

    Semaphore_Handler handler(semaphore);
    for (unsigned int c = 0; c < SOS::RETRIES; c++) {
        Alarm alarm(SOS::TIMEOUT, &handler, 1);
        
        semaphore->p();

        mutex->lock();
        if (msg_id != pacote.id) {
            mutex->unlock();
            return 1;
        }
        mutex->unlock();

        cout << "RETRIE " << c << " | msg_id: " << msg_id << endl;
        sos->send(addr, &pacote, size);
    }

    return 0;
}
int SOS::SOS_Communicator::receive(char data[], unsigned int size)
{
    semaphore->p();

    Pacote pacote;
    sos->receive(&pacote, sizeof(Pacote));

    memcpy(data, pacote.data, size);
    return 0;
}
void SOS::SOS_Communicator::update(Observed * obs, const unsigned int& prot, Buffer * buf)
{
    OStream cout;

    Pacote pacote;
    memcpy(&pacote, buf->frame()->data<void>(), sizeof(Pacote));

    // pacote.type == 0 => nao eh um ACK
    if (pacote.id <= msg_id && pacote.type == 0) {
        Pacote ack;
        ack.port_destination = pacote.port_source;
        ack.port_source = pacote.port_destination;
        ack.id = pacote.id;
        ack.size = 0;
        ack.type = 1; // ACK

        NIC_Address address;
        address = buf->frame()->src();

        if (msg_id % 100 != 0) { // Forca reenvio para testar RETRIE
            if (msg_id < 1000) { // Forca reenvios para testar TIMEOUT
                sos->send(address, &ack, 0);
            }
        }
    }

    if (pacote.id == msg_id) {
        mutex->lock();
        msg_id++;
        mutex->unlock();
        semaphore->v();
    }

    // pacote.type == 1 => ACK
    if (pacote.id != msg_id || pacote.type == 1) {
        nic->free(buf);
    }

    // pacote.type == 1 => ACK
    if (pacote.type == 1) {
        cout << "ACK " << pacote.id << " ";
    }
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
}
void SOS::send(const NIC_Address& adress, Pacote* pacote, unsigned int size)
{
    nic_send(adress, pacote, size);
}
void SOS::receive(Pacote* pacote, unsigned int size)
{
    nic_receive(pacote, size);
}
void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Ethernet::Buffer * buf)
{
    unsigned int port_dest;
    memcpy(&port_dest, buf->frame()->data<void>(), sizeof(unsigned int));

    if (!notify(port_dest, buf)) {
        nic->free(buf);
    }
}
void SOS::nic_send(const NIC_Address& address, Pacote* pacote, unsigned int size)
{
    SOS::nic->send(address, protocol, pacote, sizeof(Pacote));
}
void SOS::nic_receive(Pacote* pacote, unsigned int size)
{
    NIC<Ethernet>::Protocol prot;
    SOS::NIC_Address src;

    SOS::nic->receive(&src, &prot, pacote, size);
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

__END_SYS


#endif