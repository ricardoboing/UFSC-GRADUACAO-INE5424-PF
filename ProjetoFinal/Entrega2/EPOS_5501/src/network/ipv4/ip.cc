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







int func_timeout(SOS* sos, unsigned char data[], unsigned int size, bool* timeout, bool *msg)
{
    sos->timeout(data, size, timeout, msg);
    return 0;
}
void SOS::timeout(unsigned char data[], unsigned int size, bool* timeout, bool *msg)
{
    OStream cout;

    for (unsigned int c = 0; c < Traits<SOS>::RETRIES; c++) {
        Delay(Traits<SOS>::TIMEOUT*100000);
        
        mutex->lock();
        if (*msg) {
            mutex->unlock();
            return;
        }
        mutex->unlock();

        if (operacao == SEND) {
            nic_send(data, size);
        }
        
        cout << "TIMEOUT " << c << endl;
    }

    mutex->lock();
    *timeout = true;
    semaphore->v();
    mutex->unlock();
}

int SOS::send(char data[],unsigned int size, char addr_dest[], unsigned short port_dest)
{
    using namespace EPOS;
    OStream cout;

    mutex->lock();
    operacao = SEND;
    mutex->unlock();
    unsigned int n_frags = 1;
    unsigned int frag_size = size;

    if (size > (nic->mtu() - header))
    {
        frag_size = (nic->mtu() - header);
        n_frags = size/(nic->mtu() - header)+1;
    }
    char dest[6];
    char * token = strchr(addr_dest, ':');
    for(unsigned int i = 0; i < 6; i++, ++token, addr_dest = token, token = strchr(addr_dest, ':'))
        dest[i] = atol(addr_dest);
    
    bool *timeout = new bool();

    for(unsigned int i=0; i<n_frags;i++){
        if(i== n_frags-1){
            frag_size = size - n_frags*(nic->mtu() - header);
        }
        unsigned char data_pack[header+size];
        make_pack(data_pack, data, frag_size, dest, port_dest, 0x0, msg_id, i, n_frags);
        unsigned int local_msg_id = msg_id;
        msg_id++;
        nic_send(data_pack, frag_size+header);

        bool *timeout = new bool();
        bool *msg = new bool();

        new Thread(&func_timeout, this, data_pack, frag_size+header, timeout, msg);

        while (true) {
            semaphore->p();

            mutex->lock();
            
            if (*timeout) {
                mutex->unlock();
                break;
            } else {
                unsigned char ack[header];
                nic_receive(ack, header);

                if ( addr_check(ack) && ack[16] && ack[17] == local_msg_id && ack[18] == i) {
                    cout<< "ACK: Frag "<< i+1 << "/" << n_frags << endl;
                    *msg = true;
                    mutex->unlock();
                    break;
                }
            }

            mutex->unlock();
        }
    }

    mutex->lock();
    operacao = DEFAULT;
    bool retorno = !*timeout;
    mutex->unlock();

    return retorno;
}
int SOS::receive(char data[], unsigned int size)
{
    using namespace EPOS;
    OStream cout;
    mutex->lock();
    operacao = RCV;
    mutex->unlock();

    bool *timeout = new bool();
    bool *msg = new bool();
    //deve ser removido?
    //new Thread(&func_timeout, this, data, size, timeout, msg);

    while (true) {
        semaphore->p();

        mutex->lock();
        if (*timeout) {
            mutex->unlock();
            break;
        } else {
            unsigned char data_pack[header+size];
            nic_receive(data_pack, header+size);
            if(addr_check(data_pack)){
                cout<< "addr match" <<endl;
                *msg = true;
                unsigned char ack[header];
                char addr_dest[6];
                for(int i = 0; i < 6; i++)
                    addr_dest[i] = data_pack[i];
                

                unsigned short port_dest = 0;
                port_dest = data_pack[7] << 8 | data_pack[6];
                make_pack(ack,data, 0, addr_dest, port_dest,0x1, data_pack[17],data_pack[18],data_pack[19]);
                nic_send(ack, header);
                for(unsigned i = 0; i < size; i++){
                    data[i] = data_pack[i+header];
                }
                mutex->unlock();
                break;
            }
        }
        mutex->unlock();
    }

    mutex->lock();
    operacao = DEFAULT;
    bool retorno = !*timeout;
    mutex->unlock();

    return retorno;
}
void SOS::update(NIC<Ethernet>::Observed * obs, const NIC<Ethernet>::Protocol & prot, Buffer * buf)
{
    OStream cout;

    mutex->lock();
    cout << "update" << operacao << endl;

    if (operacao != DEFAULT) {
        semaphore->v();
    } else {
        nic->free(buf);
    }
    
    mutex->unlock();
}
SOS::SOS(unsigned short porta)
{
    port = porta;
    protocol = 0x8888;
    operacao = DEFAULT;
    header = 24;
    msg_id = 0;

    mutex = new Mutex();
    semaphore = new Semaphore(0);

    SOS::nic = Traits<Ethernet>::DEVICES::Get<0>::Result::get(0);
    SOS::nic->attach(this, protocol);
}
SOS::~SOS()
{
    SOS::nic->detach(this, protocol);
    delete mutex;
    delete semaphore;
}
void SOS::nic_send(unsigned char data[], unsigned int size)
{
    SOS::nic->send(SOS::broadcast(), protocol, data, size);
}
void SOS::nic_receive(unsigned char data[], unsigned int size)
{
    NIC<Ethernet>::Address src;
    NIC<Ethernet>::Protocol prot;
    SOS::nic->receive(&src, &prot, data, size);
}

void SOS::make_pack(unsigned char pack[],char data[], unsigned int size, char addr_dest[], unsigned short port_dest,unsigned char ack, unsigned char id, unsigned char frag, unsigned char n_frag){
    unsigned char port_char[sizeof(short)];
    OStream cout;
    for(int i = 0; i < 6; i++)
        pack[i] = nic_address()[i];

    *(unsigned short *)port_char = port;
    //port_char = reinterpret_cast<unsigned char>(port_dest);
    for(int i = 6; i < 8; i++)
        pack[i] = port_char[i-6];

    for(unsigned int i = 0; i < 6; i++)
        pack[i+8] = addr_dest[i];

    *(unsigned short *)port_char = port_dest;
    for(int i = 14; i<16;i++)
        pack[i] = port_char[i-14];

    pack[16] = ack;///ack;
    pack[17] = id;//ID;
    pack[18] = frag;// frag atual;
    pack[19] = n_frag; // total; 

    unsigned char size_char[sizeof(int)];
    *(unsigned int *)size_char = size;
    
    for(int i = 20; i<24;i++)
        pack[i] = size_char[i-20];
    for(unsigned int i = 0; i<size;i++){
         pack[i+header] = data[i]; 
     }

     // for(int i = 0; i < 22; i++)
     //                cout<< pack[i]<<endl;
}

bool SOS::addr_check(unsigned char pack[]){
    OStream cout;
    for(int i = 0; i < 6; i++){

        if(pack[i+8] != nic_address()[i])
            return 0;
    }
    unsigned char port_char[sizeof(short)];
    *(unsigned short *)port_char = port;    
    for(int i = 0; i < 2; i++){

        if(pack[i+14] != port_char[i])
            return 0;
    }
    return 1;
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