#include <architecture.h>
#include <utility/convert.h>
#include <network/ethernet.h>

class SOS {
public:

protected:

private:
	
}

// PSW: ocorrencias de carry e overflow
// EPROM: erasable programable ROM

class i82557a: public NIC<Ethernet>, private SOS {
public:
	// MDI
	// Sem suporte para energia

	// CSR: Control / Status Register
	enum {
		SCB_CW,
		SCB_SW,
		SCB_GP,
		PORT,
		EEPROM_CR,
		MDI_CR,
		RX_DMA_BC,
		PMDR,
		FLOW_CR,
		GENERAL_S,
		GENERAL_C,
		F_EVENT_REGISTER,
		F_EVENT_MASK_REGISTER,
		F_PRESENT_STATE_REGISTER,
		FORCE_EVENT_REGISTER,


	};

	enum {
		
	};
protected:

private:

}