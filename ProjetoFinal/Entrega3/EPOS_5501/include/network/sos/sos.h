// EPOS SOS Protocol Declarations

#ifndef __sos_h
#define __sos_h

#include <system/config.h>

#ifdef __sos__

#include <utility/bitmap.h>
#include <machine/nic.h>
#include <process.h>
#include <time.h>
#include <synchronizer.h>
#include <machine/uart.h>
#include <utility/geometry.h>

__BEGIN_SYS
class GPS_Driver{
protected:
    UART * uart;
    const int eof = 37; // ascii for "%"
    int count;
public:
    GPS_Driver();
    ~GPS_Driver();
    Point<int,3> get_coord();
protected:
    char* get_data_from_serial();

};


class SOS: private NIC<Ethernet>::Observer {
public:
    typedef Ethernet::Buffer Buffer;
    typedef NIC<Ethernet>::Address NIC_Address;

    typedef Data_Observer<Buffer, unsigned int> Observer;
    typedef Data_Observed<Buffer, unsigned int> Observed;

    typedef Alarm::Tick Tick;
    
    template<typename T>
    using Doubly_Linked = List_Elements::Doubly_Linked<T>;

    static SOS* ponteiro;

protected:
    static const unsigned int RETRIES = Traits<SOS>::RETRIES;
    static const unsigned int TIMEOUT = Traits<SOS>::TIMEOUT*100000;

    enum {
        MSG_TYPE_DEFAULT = 0,
        MSG_TYPE_ACK = 1
    };

    struct Pacote {
        unsigned int port_destination = 0;
        unsigned int id = 0;
        unsigned int size = 0;
        unsigned int port_source = 0;
        Tick elapsed = 0;
        Point<int, 3> coordinates;
        bool valid_position = false;
        unsigned int type = MSG_TYPE_DEFAULT;
        char data[1000];
    };
    struct Ancora{
        bool valid_position = false;
        Point<int, 3> position;
        int distance = 0;

    };

public:
    class SOS_Communicator: private SOS::Observer {
    public:
        SOS_Communicator(unsigned int porta);
        ~SOS_Communicator();

        int send(const char* address, unsigned int port_dest, char data[], unsigned int size);
        int receive(char data[], unsigned int size);

    protected:
        SOS* sos;

        Semaphore* semaphore;
        Mutex* mutex;

        unsigned int port;
        unsigned int msg_id;

        struct Cliente {
            NIC_Address address;
            unsigned int id = 0;
        };

        List<Cliente>* clientes;
        List<Pacote>* pacotes;

        Cliente* client(NIC_Address& address);

        void update(Observed * obs, const unsigned int& prot, Buffer * buf);
    };

    SOS();
    ~SOS();

    static void init(unsigned int unit);

    void send(const NIC_Address& address, Pacote* pacote);
    
    void attach(Observer* obs, const unsigned int& port) { _observed->attach(obs, port); }
    void detach(Observer* obs, const unsigned int& port) { _observed->detach(obs, port); }

    static NIC_Address nic_address() { return SOS::nic->address(); }

protected:
    static NIC<Ethernet> * nic;
    Observed *_observed;
    unsigned short protocol = 0x8888;

    static Tick tick1;
    static Tick tick2;
    static Tick tick3;
    static Tick ultimo_elapsed;
    GPS_Driver * gps;
    bool valid_position;
    Point<int, 3> position;

    Ancora anchor1;
    Ancora anchor2;
    Ancora anchor3;
    Point<int, 3> spected_position;

    void update(Ethernet::Observed * obs, const Ethernet::Protocol & prot, Ethernet::Buffer * buf);

private:
    bool notify(const unsigned int& port, Buffer *buf) { return _observed->notify(port, buf); }

};



__END_SYS

#endif
#endif