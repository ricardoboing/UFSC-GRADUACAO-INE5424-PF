// EPOS IP Protocol Implementation

#include <utility/string.h>
#include <network/ipv4/arp.h>
#include <network/ipv4/ip.h>
#include <network/ipv4/udp.h>
#include <network/ipv4/dhcp.h>
#include <system.h>

#ifdef __ipv4__

__BEGIN_SYS

// Class attributes
unsigned short IP::Header::_next_id = 0;
IP * IP::_networks[];
IP::Router IP::_router;
IP::Reassembling IP::_reassembling;
IP::Observed IP::_observed;

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














int SOS::send(char data[])
{
    using namespace EPOS;
    OStream cout;

    mutex->lock();
    operacao = SEND;
    ackk = false;
    mutex->unlock();

    nic_send(data);

    int retorno = 0;

    for (unsigned int c = 0; c < Traits<SOS>::RETRIES; c++) {
        Delay(Traits<SOS>::TIMEOUT*100000);

        mutex->lock();
        if (msg) {
            if (ackk) {
                cout << "ACK" << endl;
                retorno = 1;
                msg = false;
                mutex->unlock();
                break;
            }

            msg = false;
            cout << "NAO ACK" << endl;
        }

        data[c]++;
        nic_send(data);
        mutex->unlock();
        cout << "TIMEOUT " << c << endl;
    }
    
    mutex->lock();
    operacao = DEFAULT;
    mutex->unlock();

    return retorno;
}
int SOS::receive(char dado[], unsigned int size)
{
    using namespace EPOS;
    OStream cout;

    mutex->lock();
    operacao = RCV;
    msg = false;
    mutex->unlock();

    int retorno = 0;

    for (unsigned int c = 0; c < Traits<SOS>::RETRIES; c++) {
        Delay(Traits<SOS>::TIMEOUT*100000);

        mutex->lock();
        if (msg) {
            cout << "MSG" << endl;
            retorno = 1;
            mutex->unlock();
            dado[0] = data[0];
            break;
        }

        mutex->unlock();
        cout << "TIMEOUT " << c << endl;
    }
    
    mutex->lock();
    operacao = DEFAULT;
    mutex->unlock();

    return retorno;
}
void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Buffer * buf)
{
    using namespace EPOS;
    OStream cout;
    cout << "update" << endl;

    mutex->lock();
    switch(operacao) {
        case SEND:
            cout << "update SEND" << endl;
            update_send();
            mutex->unlock();
            break;
        case RCV:
            cout << "update RCV" << endl;
            update_receive();
            mutex->unlock();
            break;
        default:
            cout << "update DEFAULT" << endl;
            nic->free(buf);
            mutex->unlock();
            break;
    }
}
void SOS::update_receive() {
    msg = true;
    nic_receive(data, 1);
}
void SOS::update_send() {
    msg = true;
    nic_receive(data, 1);
    
    if (data[0] == '3') {
        ackk = true;
    }
}
/*
void SOS::receive(char data[], unsigned int size)
{   
    mutex->lock();
    operacao = RCV;
    mutex->unlock();

    semaphoreReceive->p();

    nic_receive(data, size);

    char ack[1];
    
    
    //Delay(600000);
    memset(ack, '0', 1);
    nic_send(ack);

    memset(ack, '1', 1);
    nic_send(ack);
    
    memset(ack, '2', 1);
    nic_send(ack);
    
    //memset(ack, '3', 1);
    //nic_send(ack);




    mutex->lock();
    operacao = DEFAULT;
    mutex->unlock();
}*/
SOS::SOS(unsigned int port)
{
    port = port;
    protocol = 0x8888;
    operacao = DEFAULT;

    mutex = new Mutex();

    SOS::nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    SOS::nic->attach(this, protocol);
}
SOS::~SOS()
{
    SOS::nic->detach(this, protocol);
    delete mutex;
}
void SOS::nic_send(char data[])
{
    SOS::nic->send(SOS::broadcast(), protocol, data, 1);
}
void SOS::nic_receive(char data[], unsigned int size)
{
    NIC<Ethernet>::Address src;
    NIC<Ethernet>::Protocol prot;
    SOS::nic->receive(&src, &prot, data, size);
}
void SOS::statistics()
{
    using namespace EPOS;
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