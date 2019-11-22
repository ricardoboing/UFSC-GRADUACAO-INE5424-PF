// EPOS PC UART Mediator Test Program

#include <utility/ostream.h>
#include <machine/uart.h>
#include <time.h>
using namespace EPOS;

int main()
{
    OStream cout;

    cout << "UART Socket test\n" << endl;
    //primeiro campo é a porta. no caso 1 representa COM2
    //outros nao parecem influenciar em nada.
    //ultimo campo nao parece alterar nd.
    UART * uart = new UART(1, 115200, 8, 0, 1); // using 0 for network, 1 is default for console (-serial mon:stdio)

    cout << "Loopback transmission test (conf = 115200 8N1):";
    uart->loopback(false);//se for true eu nao entendi mas parece que ele reconhece um put como um get

    for(int i = 0; i < 100; i++) {
    	Delay (5000000);
        //int c = uart->get(); // getting ASCII input as int
        //ele so recebe nesse formata pelo que entendi, um caractere por vez,
        //no envio pode ser enviado mais de um caractere mas dai ele vai necessidar de um get por ascii
        //cout << "received (" << c << ")" << endl;
        uart->put(i + 48); // sending "0", "1", "2" in ASCII
        // so funciona com o servidor RECV_DATA_FROM_SERVER = True
        cout  << "sent (" << i << ")" << endl;
    }
    cout << "end!" << endl;
    Delay (5000000);
    return 0;
}
