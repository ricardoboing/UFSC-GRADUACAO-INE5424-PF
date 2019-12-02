#include <network/sos/sos.h>
#include <machine/nic.h>

#ifdef __sos__

__BEGIN_SYS

void SOS::init(unsigned int unit) {
    ponteiro = new (SYSTEM) SOS();
    tick1 = 0;
    tick2 = 0;
    tick3 = 0;
    ultimo_elapsed = 0;
}

__END_SYS

#endif
