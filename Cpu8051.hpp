#ifndef CPU8051_HPP
#define CPU8051_HPP

#include "HexFile.hpp"
#include "Bits.hpp"

#include <QtEndian>

class Cpu8051
{
public:
	enum ERR { BREAKPOINT = -3, ERR_UNSUPPORTED_MODE, ERR_NO_IMPLEMENTED, ERR_A5_INSTRUCTION };
	enum IPFlags { PX0, PT0, PX1, PT1, PS, IPReserved1, IPReserved2, IPReserved3 };
	enum IEFlags { EX0, ET0, EX1, ET1, ES, IEReserved1, IEReserved2, EA };
	enum PSWFlags { P, PSWReserved1, OV, RS0, RS1, F0, AC, CY };
	enum TCONFlags { IT0, IE0, IT1, IE1, TR0, TF0, TR1, TF1 };
	enum TMODFlags { M00, M01, CT0, G0, M10, M11, CT1, G1 };
	enum SCONFlags { RI, TI, RB8, TB8, REN, SM2, SM1, SM0 };
	enum P3Flags { RxD, TxD, INT0, INT1, T0, T1 };

	Cpu8051();

	/* Loads software into ROM and clears all registers */
	HexFile::HexERROR loadSoftware( const char *hex, uint32_t size );

	void clear();

	int8_t step();

	inline uint16_t getPC() const
	{
		return PC;
	}

	inline uint8_t getInstrucion( int8_t offset = -1 )
	{
		return ROM[ PC + offset ];
	}

	inline uint16_t DPTR() const
	{
		return get16b( DPL );
	}
	inline void setDPTR( uint16_t val )
	{
		set16b( DPL, val );
	}

	uint8_t inP0, inP1, inP2, inP3, inSBUF, serial_transmission;

	uint8_t &PCON, &TCON, &SCON, &TMOD, &SBUF, &IE, &IP, &PSW;
	uint8_t &TL0, &TH0, &TL1, &TH1;
	uint8_t &P0, &P1, &P2, &P3;
	uint8_t &DPL, &DPH;
	uint8_t &ACC, &B;
	uint8_t *R;
	uint8_t &SP;

	uint8_t RAM[ 0x100 ];
	uint8_t ROM[ 0x10000 ];
private:
	static inline uint16_t get16b( const uint8_t &byte )
	{
		return qFromBigEndian( ( uint16_t & )byte );
	}
	static inline void set16b( uint8_t &byte, uint16_t val )
	{
		( uint16_t & )byte = qToBigEndian( val );
	}

	inline void push8( uint8_t val )
	{
		RAM[ ++SP ] = val;
	}
	inline uint8_t pop8() const
	{
		return RAM[ SP-- ];
	}
	inline void push16( uint16_t val )
	{
		RAM[ ++SP ] = val & 0xFF;
		RAM[ ++SP ] = val >> 8;
	}
	inline uint16_t pop16() const
	{
		SP -= 2;
		return ( RAM[ SP + 2 ] << 8 ) | RAM[ SP + 1 ];
	}

	inline void initSerialTransmission( const uint8_t adr, const uint8_t cycles )
	{
		if ( adr == 0x99 )
			serial_transmission = 9 + cycles;
	}

	inline bool getBit()
	{
		const uint8_t bit_addr = ROM[ PC ];
		const uint8_t bit_nr = bit_addr % 8;
		if ( bit_addr & 0x80 )
			return Bits::getBit( getRAMByte( bit_addr - bit_nr ), bit_nr );
		return Bits::getBit( getRAMByte( bit_addr / 8 + 0x20 ), bit_nr );
	}
	inline bool getBitDirect()
	{
		const uint8_t bit_addr = ROM[ PC ];
		const uint8_t bit_nr = bit_addr % 8;
		if ( bit_addr & 0x80 )
			return Bits::getBit( RAM[ bit_addr - bit_nr ], bit_nr );
		return Bits::getBit( RAM[ bit_addr / 8 + 0x20 ], bit_nr );
	}
	inline void setBit( const bool bit )
	{
		const uint8_t bit_addr = ROM[ PC ];
		const uint8_t bit_nr = bit_addr % 8;
		if ( bit_addr & 0x80 )
			Bits::setBit( RAM[ bit_addr - bit_nr ], bit_nr, bit );
		else
			Bits::setBit( RAM[ bit_addr / 8 + 0x20 ], bit_nr, bit );
	}

	inline void AddOperation( uint16_t val, const bool PSW_C_Flag = false )
	{
		val += PSW_C_Flag && Bits::getBit( PSW, CY );
		const uint16_t sum = ACC + val;
		Bits::setBit( PSW, CY, sum > 0xFF );
		Bits::setBit( PSW, AC, ( val & 0x0F ) + ( ACC & 0x0F ) > 0x0F );
		Bits::setBit( PSW, OV, ( val < 128 && ACC < 128 && ( uint8_t )sum > 127 ) || ( val > 127 && ACC > 127 && ( uint8_t )sum < 128 ) );
		ACC = sum;
	}
	inline void SubOperation( uint16_t val )
	{
		val += Bits::getBit( PSW, CY );
		const uint8_t diff = ACC - val;
		Bits::setBit( PSW, CY, val > ACC );
		Bits::setBit( PSW, AC, ( val & 0x0F ) > ( ACC & 0x0F ) );
		Bits::setBit( PSW, OV, ( val > 127 && ACC < 128 && diff > 127 ) || ( val < 128 && ACC > 127 && diff < 128 ) );
		ACC = diff;
	}

	inline void setPC_AJMP_ACALL( const uint8_t &instr )
	{
		PC = ( PC & 0xF800 ) | ( ( ( instr & 0xE0 ) << 3 ) | ROM[ PC ] );
	}

	inline void CJNE( const uint8_t val1, const uint8_t val2 )
	{
		if ( val1 != val2 )
			PC += ( int8_t )ROM[ PC ] + 1;
		else
			++PC;
		Bits::setBit( PSW, CY, val1 < val2 );
	}

	inline void swap( uint8_t &a, uint8_t &b )
	{
		const uint8_t tmp = a;
		a = b;
		b = tmp;
	}

	inline uint8_t getRAMByte( const uint8_t adr8 )
	{
		switch ( adr8 )
		{
			case 0x80:
				return P0 & inP0;
			case 0x90:
				return P1 & inP1;
			case 0x99:
				return inSBUF;
			case 0xA0:
				return P2 & inP2;
			case 0xB0:
				return P3 & inP3;
		}
		return RAM[ adr8 ];
	}

	inline bool interrupt( const uint8_t interruptNumber )
	{
		switch ( interruptNumber )
		{
			case 0:
				return Bits::getBit( IE, EX0 ) && Bits::getBit( lastTCON, IE0 ) && interrupt( PX0, IE0, 0x0003 );
			case 1:
				return Bits::getBit( IE, ET0 ) && Bits::getBit( lastTCON, TF0 ) && interrupt( PT0, TF0, 0x000B );
			case 2:
				return Bits::getBit( IE, EX1 ) && Bits::getBit( lastTCON, IE1 ) && interrupt( PX1, IE1, 0x0013 );
			case 3:
				return Bits::getBit( IE, ET1 ) && Bits::getBit( lastTCON, TF1 ) && interrupt( PT1, TF1, 0x001B );
			case 4:
				return Bits::getBit( IE, ES ) && ( Bits::getBit( lastSCON, TI ) || Bits::getBit( lastSCON, RI ) ) && interrupt( PS, 0xFF/*Not used*/, 0x0023 );
		}
		return false;
	}
	bool interrupt( const uint8_t IPFlag, const uint8_t TCONFlag, const uint16_t PCValue );

	uint8_t processTimer( uint8_t &TL, uint8_t &TH, const uint8_t TMOD, const bool INT, const bool T, const uint8_t TF, const uint8_t cycles );

	uint16_t PC;
	uint8_t lastTCON, lastSCON, lastP3, interruptLevel, lastInterruptLevel;
};

#endif // CPU8051_HPP
