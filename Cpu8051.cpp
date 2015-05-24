#include "Cpu8051.hpp"

static const bool parityArray[ 256 ] = {
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0
};

static const uint8_t cycles[ 256 ] = { //Unimplemented: MOVX
	1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 1, 2, 4, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 1, 1, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2,
	0, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 2, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static const uint8_t interruptsPriorityTable[ 32 ][ 5 ] = {
	{ 0, 1, 2, 3, 4 }, //0
	{ 0, 1, 2, 3, 4 }, //1
	{ 1, 0, 2, 3, 4 }, //2
	{ 0, 1, 2, 3, 4 }, //3
	{ 2, 0, 1, 3, 4 }, //4
	{ 0, 2, 1, 3, 4 }, //5
	{ 1, 2, 0, 3, 4 }, //6
	{ 0, 1, 2, 3, 4 }, //7
	{ 3, 0, 1, 2, 4 }, //8
	{ 0, 3, 1, 2, 4 }, //9
	{ 1, 3, 0, 2, 4 }, //10
	{ 0, 1, 3, 2, 4 }, //11
	{ 2, 3, 0, 1, 4 }, //12
	{ 0, 2, 3, 1, 4 }, //13
	{ 1, 2, 3, 0, 4 }, //14
	{ 0, 1, 2, 3, 4 }, //15
	{ 4, 0, 1, 2, 3 }, //16
	{ 0, 4, 1, 2, 3 }, //17
	{ 1, 4, 0, 2, 3 }, //18
	{ 0, 1, 4, 2, 3 }, //19
	{ 2, 4, 0, 1, 3 }, //20
	{ 0, 2, 4, 1, 3 }, //21
	{ 1, 2, 4, 0, 3 }, //22
	{ 0, 1, 2, 4, 3 }, //23
	{ 3, 4, 0, 1, 2 }, //24
	{ 0, 3, 4, 1, 2 }, //25
	{ 1, 3, 4, 0, 2 }, //26
	{ 0, 1, 3, 4, 2 }, //27
	{ 2, 3, 4, 0, 1 }, //28
	{ 0, 2, 3, 4, 1 }, //29
	{ 1, 2, 3, 4, 0 }, //30
	{ 0, 1, 2, 3, 4 }  //31
};

/**/

Cpu8051::Cpu8051() :
	PCON( RAM[ 0x87 ] ), TCON( RAM[ 0x88 ] ), SCON( RAM[ 0x98 ] ), TMOD( RAM[ 0x89 ] ), SBUF( RAM[ 0x99 ] ), IE( RAM[ 0xA8 ] ), IP( RAM[ 0xB8 ] ), PSW( RAM[ 0xD0 ] ),
	TL0( RAM[ 0x8A ] ), TH0( RAM[ 0x8C ] ), TL1( RAM[ 0x8B ] ), TH1( RAM[ 0x8D ] ),
	P0( RAM[ 0x80 ] ), P1( RAM[ 0x90 ] ), P2( RAM[ 0xA0 ] ), P3( RAM[ 0xB0 ] ),
	DPL( RAM[ 0x82 ] ), DPH( RAM[ 0x83 ] ),
	ACC( RAM[ 0xE0 ] ), B( RAM[ 0xF0 ] ),
	SP( RAM[ 0x81 ] )
{
	memset( ROM, 0x00, sizeof ROM );
	clear();
}

HexFile::HexERROR Cpu8051::loadSoftware( const char *hex, uint32_t size )
{
	clear();
	return HexFile::parse( hex, size, ROM, sizeof ROM );
}

void Cpu8051::clear()
{
	for ( quint8 i = 0 ; i < 0x80 ; ++i )
		RAM[ i ] = qrand() % 0xFF;
	memset( RAM + 0x80, 0x00, sizeof RAM >> 1 );
	inP0 = inP1 = inP2 = inP3 = P0 = P1 = P2 = P3 = lastP3 = 0xFF;
	serial_transmission = interruptLevel = lastInterruptLevel = 0;
	PC = lastTCON = lastSCON = inSBUF = 0x00;
	R = RAM + 0x00;
	SP = 0x07;
}

int8_t Cpu8051::step() //Unimplemented: MOVX, timers (mode 3), serial transmition (UART, DJNZ - I don't know how to implement DJNZ on SBUF)
{
	uint8_t tmp;
	uint16_t tmp16;

	bool canInterrupt = Bits::getBit( IE, EA );
	const uint8_t port3 = inP3 & P3;
	const uint8_t &instr = ROM[ PC++ ];
	uint8_t cycles = ::cycles[ instr ];

	switch ( instr )
	{
		case 0x00: //NOP
			break;
		case 0x01: case 0x21: case 0x41: case 0x61: case 0x81: case 0xA1: case 0xC1: case 0xE1: //AJMP adr11
			setPC_AJMP_ACALL( instr );
			break;
		case 0x02: //LJMP
			PC = get16b( ROM[ PC ] );
			break;
		case 0x03: //RR A
			ACC = ( ACC << 7 ) | ( ACC >> 1 );
			break;
		case 0x04: //INC A
			++ACC;
			break;
		case 0x05: //INC adr8
			++RAM[ ROM[ PC ] ];
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x06: case 0x07: //INC @Ri
			++RAM[ R[ instr - 0x06 ] ];
			break;
		case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F: //INC Rr
			++R[ instr - 0x08 ];
			break;
		case 0x11: case 0x31: case 0x51: case 0x71: case 0x91: case 0xB1: case 0xD1: case 0xF1: //ACALL adr11
			push16( PC + 1 );
			setPC_AJMP_ACALL( instr );
			break;
		case 0x12: //LCALL
			push16( PC + 2 );
			PC = get16b( ROM[ PC ] );
			break;
		case 0x13: //RRC A
			tmp = Bits::getBit( PSW, CY ) << 7;
			Bits::setBit( PSW, CY, Bits::getBit( ACC, 0 ) );
			ACC = ( ACC >> 1 ) | tmp;
			break;
		case 0x14: //DEC A
			--ACC;
			break;
		case 0x15: //DEC adr8
			--RAM[ ROM[ PC ] ];
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x16: case 0x17: //DEC @Ri
			--RAM[ R[ instr - 0x16 ] ];
			break;
		case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F: //DEC Rr
			--R[ instr - 0x18 ];
			break;

		case 0x22: //RET
			PC = pop16();
			break;
		case 0x32: //RETI
			canInterrupt = false;
			interruptLevel = lastInterruptLevel;
			if ( lastInterruptLevel ) //Only two priority levels, so I can decrement it
				--lastInterruptLevel;
			PC = pop16();
			break;

		case 0x23: //RL A
			ACC = ( ACC >> 7 ) | ( ACC << 1 );
			break;

		case 0x24: //ADD A, #n
			AddOperation( ROM[ PC++ ] );
			break;
		case 0x25: //ADD A, adr8
			AddOperation( getRAMByte( ROM[ PC++ ] ) );
			break;
		case 0x26: case 0x27: //ADD A, @Ri
			AddOperation( RAM[ R[ instr - 0x26 ] ] );
			break;
		case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F: //ADD A, Rr
			AddOperation( R[ instr - 0x28 ] );
			break;

		case 0x33: //RLC A
			tmp = Bits::getBit( PSW, CY );
			Bits::setBit( PSW, CY, Bits::getBit( ACC, 7 ) );
			ACC = ( ACC << 1 ) | tmp;
			break;

		case 0x34: //ADDC A, #n
			AddOperation( ROM[ PC++ ], true );
			break;
		case 0x35: //ADDC A, adr8
			AddOperation( getRAMByte( ROM[ PC++ ] ), true );
			break;
		case 0x36: case 0x37: //ADDC A, @Ri
			AddOperation( RAM[ R[ instr - 0x36 ] ], true );
			break;
		case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: //ADDC A, Rr
			AddOperation( R[ instr - 0x38 ], true );
			break;

		case 0x10: //JBC bit, d
			if ( getBitDirect() )
			{
				setBit( false );
				PC += ( int8_t )ROM[ PC + 1 ] + 2;
			}
			else
				PC += 2;
			break;
		case 0x20: //JB bit, d
		case 0x30: //JNB bit, d
			if ( getBit() != ( instr == 0x30 ) )
				PC += ( int8_t )ROM[ PC + 1 ] + 2;
			else
				PC += 2;
			break;

		case 0x40: //JC d
		case 0x50: //JNC d
			if ( Bits::getBit( PSW, CY ) != ( instr == 0x50 ) )
				PC += ( int8_t )ROM[ PC ] + 1;
			else
				++PC;
			break;
		case 0x60: //JZ d
		case 0x70: //JNZ d
			if ( !ACC != ( instr == 0x70 ) )
				PC += ( int8_t )ROM[ PC ] + 1;
			else
				++PC;
			break;

		case 0x42: //ORL adr8, A
			RAM[ ROM[ PC ] ] |= ACC;
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x43: //ORL adr8, #n
			RAM[ ROM[ PC ] ] |= ROM[ PC + 1 ];
			initSerialTransmission( ROM[ PC ], cycles );
			PC += 2;
			break;
		case 0x44: //ORL A, #n
			ACC |= ROM[ PC++ ];
			break;
		case 0x45: //ORL A, adr8
			ACC |= getRAMByte( ROM[ PC++ ] );
			break;
		case 0x46: case 0x47: //ORL A, @Ri
			ACC |= RAM[ R[ instr - 0x46 ] ];
		case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F: //ORL A, Rr
			ACC |= R[ instr - 0x48 ];
			break;

		case 0x52: //ANL adr8, A
			RAM[ ROM[ PC ] ] &= ACC;
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x53: //ANL adr8, #n
			RAM[ ROM[ PC ] ] &= ROM[ PC + 1 ];
			initSerialTransmission( ROM[ PC ], cycles );
			PC += 2;
			break;
		case 0x54: //ANL A, #n
			ACC &= ROM[ PC++ ];
			break;
		case 0x55: //ANL A, adr8
			ACC &= getRAMByte( ROM[ PC++ ] );
			break;
		case 0x56: case 0x57: //ANL A, @Ri
			ACC &= RAM[ R[ instr - 0x56 ] ];
		case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F: //ANL A, Rr
			ACC &= R[ instr - 0x58 ];
			break;

		case 0x62: //XRL adr8, A
			RAM[ ROM[ PC ] ] ^= ACC;
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x63: //XRL adr8, #n
			RAM[ ROM[ PC ] ] ^= ROM[ PC + 1 ];
			initSerialTransmission( ROM[ PC ], cycles );
			PC += 2;
			break;
		case 0x64: //XRL A, #n
			ACC ^= ROM[ PC++ ];
			break;
		case 0x65: //XRL A, adr8
			ACC ^= getRAMByte( ROM[ PC++ ] );
			break;
		case 0x66: case 0x67: //XRL A, @Ri
			ACC ^= RAM[ R[ instr - 0x66 ] ];
		case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F: //XRL A, Rr
			ACC ^= R[ instr - 0x68 ];
			break;

		case 0x72: //ORL C, bit
		case 0xA0: //ORL C, !bit
			Bits::setBit( PSW, CY, Bits::getBit( PSW, CY ) || ( getBit() != ( instr == 0xA0 ) ) );
			++PC;
			break;

		case 0x82: //ANL C, bit
		case 0xB0: //ANL C, !bit
			Bits::setBit( PSW, CY, Bits::getBit( PSW, CY ) && ( getBit() != ( instr == 0xB0 ) ) );
			++PC;
			break;

		case 0x73: //JMP @A+DPTR
			PC = ACC + DPTR();
			break;
		case 0x74: //MOV A, #n
			ACC = ROM[ PC++ ];
			break;
		case 0x75: //MOV adr8, #n
			RAM[ ROM[ PC ] ] = ROM[ PC + 1 ];
			initSerialTransmission( ROM[ PC ], cycles );
			PC += 2;
			break;
		case 0x76: case 0x77: //MOV @Ri, #n
			RAM[ R[ instr - 0x76 ] ] = ROM[ PC++ ];
			break;
		case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F: //MOV Rr, #n
			R[ instr - 0x78 ] = ROM[ PC++ ];
			break;
		case 0x80: //SJMP d
			PC += ( int8_t )ROM[ PC ] + 1;
			break;
		case 0x84: //DIV AB
			Bits::setBit( PSW, CY, false );
			Bits::setBit( PSW, OV, !B );
			if ( B )
			{
				tmp = ACC % B;
				ACC = ACC / B;
				B = tmp;
			}
			break;
		case 0x85: //MOV adr8, adr8
			RAM[ ROM[ PC + 1 ] ] = getRAMByte( ROM[ PC ] );
			initSerialTransmission( ROM[ PC + 1 ], cycles );
			PC += 2;
			break;
		case 0x86: case 0x87: //MOV adr8, @Ri
			RAM[ ROM[ PC ] ] = RAM[ R[ instr - 0x86 ] ];
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: //MOV adr8, Rr
			RAM[ ROM[ PC ] ] = R[ instr - 0x88 ];
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0x90: //MOV DPTR, adr16
			setDPTR( get16b( ROM[ PC ] ) );
			PC += 2;
			break;
		case 0x92: //MOV bit, C
			setBit( Bits::getBit( PSW, CY ) );
			++PC;
			break;

		case 0x83: //MOVC A, @A+PC
			ACC = ROM[ ( uint16_t )( ACC + PC ) ];
			break;
		case 0x93: //MOVC A, @A+DPTR
			ACC = ROM[ ( uint16_t )( ACC + DPTR() ) ];
			break;

		case 0x94: //SUBB A, #n
			SubOperation( ROM[ PC++ ] );
			break;
		case 0x95: //SUBB A, adr8
			SubOperation( getRAMByte( ROM[ PC++ ] ) );
			break;
		case 0x96: case 0x97: //SUBB A, @Ri
			SubOperation( RAM[ R[ instr - 0x96 ] ] );
			break;
		case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F: //SUBB A, Rr
			SubOperation( R[ instr - 0x98 ] );
			break;

		case 0xA2: //MOV C, bit
			Bits::setBit( PSW, CY, getBit() );
			++PC;
			break;

		case 0xA3: //INC DPTR
			setDPTR( DPTR() + 1 );
			break;

		case 0xA4: //MUL AB
			tmp16 = ACC * B;
			ACC = tmp16 & 0xFF;
			B = tmp16 >> 8;
			Bits::setBit( PSW, CY, false );
			Bits::setBit( PSW, OV, tmp16 > 0xFF );
			break;

		case 0xA6: case 0xA7: //MOV @Ri, adr8
			RAM[ R[ instr - 0xA6 ] ] = getRAMByte( ROM[ PC++ ] );
			break;
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: //MOV Rn, adr8
			R[ instr - 0xA8 ] = getRAMByte( ROM[ PC++ ] );
			break;

		case 0xB2: //CPL bit
			setBit( !getBitDirect() );
			++PC;
			break;
		case 0xB3: //CPL C
			Bits::toggleBit( PSW, CY );
			break;

		case 0xB4: //CJNE A, #n, d
			CJNE( ACC, ROM[ PC++ ] );
			break;
		case 0xB5: //CJNE A, adr8, d
			CJNE( ACC, getRAMByte( ROM[ PC++ ] ) );
			break;
		case 0xB6: case 0xB7: //CJNE @Ri, #n, d
			CJNE( RAM[ R[ instr - 0xB6 ] ], ROM[ PC++ ] );
			break;
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF: //CJNE Rn, #n, d
			CJNE( R[ instr - 0xB8 ], ROM[ PC++ ] );
			break;

		case 0xC0: //PUSH adr8
			if ( ROM[ PC ] == 0x99 )
				push8( inSBUF );
			else
				push8( RAM[ ROM[ PC ] ] );
			++PC;
			break;
		case 0xC2: //CLR bit
			setBit( false );
			++PC;
			break;
		case 0xC3: //CLR C
			Bits::setBit( PSW, CY, false );
			break;
		case 0xC4: //SWAP A
			ACC = ( ACC << 4 ) | ( ACC >> 4 );
			break;

		case 0xC5: //XCH A, adr8
			tmp = getRAMByte( ROM[ PC ] );
			RAM[ ROM[ PC ] ] = ACC;
			ACC = tmp;
			++PC;
			break;
		case 0xC6: case 0xC7: //XCH A, @Ri
			swap( ACC, RAM[ R[ instr - 0xC6 ] ] );
			break;
		case 0xC8: case 0xC9: case 0xCA: case 0xCB: case 0xCC: case 0xCD: case 0xCE: case 0xCF: //XCH A, Rr
			swap( ACC, R[ instr - 0xC8 ] );
			break;

		case 0xD0: //POP adr8
			RAM[ ROM[ PC ] ] = pop8();
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0xD2: //SETB bit
			setBit( true );
			++PC;
			break;
		case 0xD3: //SETB C
			Bits::setBit( PSW, CY, true );
			break;
		case 0xD4: //DA A
			if ( ( ACC & 0x0F ) > 9 || Bits::getBit( PSW, AC ) )
			{
				ACC += 0x06;
				if ( ( ACC & 0x0F ) + 0x06 > 0x0F )
					Bits::setBit( PSW, CY, true );
			}
			if ( ( ACC >> 4 ) > 9 || Bits::getBit( PSW, CY ) )
			{
				ACC += 0x60;
				if ( ( ACC >> 4 ) + 0x06 > 0x0F )
					Bits::setBit( PSW, CY, true );
			}
			break;

		case 0xD5: //DJNZ adr8, d, TODO: SBUF ??
			if ( --RAM[ ROM[ PC++ ] ] )
				PC += ( int8_t )ROM[ PC ] + 1;
			else
				++PC;
			break;

		case 0xD6: case 0xD7: //XCHD A, @Ri
			tmp = RAM[ R[ instr - 0xD6 ] ];
			RAM[ R[ instr - 0xD6 ] ] = ( RAM[ R[ instr - 0xD6 ] ] & 0xF0 ) | ( ACC & 0x0F );
			ACC = ( ACC & 0xF0 ) | ( tmp & 0x0F );
			break;

		case 0xD8: case 0xD9: case 0xDA: case 0xDB: case 0xDC: case 0xDD: case 0xDE: case 0xDF: //DJNZ Rr, d
			if ( --R[ instr - 0xD8 ] )
				PC += ( int8_t )ROM[ PC ] + 1;
			else
				++PC;
			break;
		case 0xE4: //CLR A
			ACC = 0;
			break;
		case 0xE5: //MOV A, adr8
			ACC = getRAMByte( ROM[ PC++ ] );
			break;
		case 0xE6: case 0xE7: //MOV A, @Ri
			ACC = RAM[ R[ instr - 0xE6 ] ];
			break;
		case 0xE8: case 0xE9: case 0xEA: case 0xEB: case 0xEC: case 0xED: case 0xEE: case 0xEF: //MOV A, Rr
			ACC = R[ instr - 0xE8 ];
			break;
		case 0xF4: //CPL A
			ACC = ~ACC;
			break;
		case 0xF5: //MOV adr8, A
			RAM[ ROM[ PC ] ] = ACC;
			initSerialTransmission( ROM[ PC++ ], cycles );
			break;
		case 0xF6: case 0xF7: //MOV @Ri, A
			RAM[ R[ instr - 0xF6 ] ] = ACC;
			break;
		case 0xF8: case 0xF9: case 0xFA: case 0xFB: case 0xFC: case 0xFD: case 0xFE: case 0xFF: //MOV Rn, A
			R[ instr - 0xF8 ] = ACC;
			break;

		case 0xA5:
			return ERR_A5_INSTRUCTION;
		default:
			return ERR_NO_IMPLEMENTED;
	}

	R = RAM + ( Bits::getBit( PSW, RS1 ) << 4 | Bits::getBit( PSW, RS0 ) << 3 ); //Working register bank
	Bits::setBit( PSW, P, parityArray[ ACC ] ); //number of "1" in Acc + Parity Flag is always even

	if ( canInterrupt ) //Interrupts enabled and not after RETI
	{
		const uint8_t interruptsPriority = IP & 0x0F;
		for ( unsigned i = 0 ; i < 5 ; ++i )
			if ( interrupt( interruptsPriorityTable[ interruptsPriority ][ i ] ) )
			{
				cycles += 2;
				break;
			}
	}

	if ( Bits::getBit( lastTCON, TR0 ) ) //Timer 0 enabled
		cycles = processTimer( TL0, TH0, TMOD & 0x0F, Bits::getBit( lastP3, INT0 ), Bits::getBit( lastP3, T0 ) && !Bits::getBit( port3, T0 ), TF0, cycles );
	if ( Bits::getBit( lastTCON, TR1 ) ) //Timer 1 enabled
		cycles = processTimer( TL1, TH1, TMOD >> 4,   Bits::getBit( lastP3, INT1 ), Bits::getBit( lastP3, T1 ) && !Bits::getBit( port3, T1 ), TF1, cycles );

	Bits::setBit( TCON, IE0, !Bits::getBit( port3, INT0 ) );
	Bits::setBit( TCON, IE1, !Bits::getBit( port3, INT1 ) );

	lastTCON = TCON;
	lastSCON = SCON;
	lastP3 = port3;

	return cycles;
}

bool Cpu8051::interrupt( const uint8_t IPFlag, const uint8_t TCONFlag, const uint16_t PCValue )
{
	const uint8_t newInterruptLevel = Bits::getBit( IP, IPFlag ) + 1;
	if ( newInterruptLevel > interruptLevel )
	{
		lastInterruptLevel = interruptLevel;
		interruptLevel = newInterruptLevel;
		if ( TCONFlag <= 7 )
			Bits::setBit( TCON, TCONFlag, false );
		push16( PC );
		PC = PCValue;
		return true;
	}
	return false;
}

uint8_t Cpu8051::processTimer( uint8_t &TL, uint8_t &TH, const uint8_t TMOD, const bool INT, const bool T, const uint8_t TF, const uint8_t cycles )
{
	if ( !Bits::getBit( TMOD, G0 ) || INT ) //GATE
	{
		uint16_t prevTimer = 0, timer = 0;
		uint8_t toAdd = Bits::getBit( TMOD, CT0 ) ? T : cycles; //Counter/Timer
		switch ( TMOD & 0x03 ) //mode
		{
			case 0: //licznik 13bit
				prevTimer = timer = ( TH << 5 ) | ( TL & 0x1F );
				timer = ( timer + toAdd ) % 0x2000;
				TL = ( TL & 0xE0 ) | ( timer & 0x1F );
				TH = timer >> 5;
				break;
			case 1: //licznik 16bit
				prevTimer = timer = ( TH << 8 ) | TL;
				timer += toAdd;
				TL = timer & 0x00FF;
				TH = timer >> 8;
				break;
			case 2: //licznik 8bit ze stanem początkowym TH
				prevTimer = TL;
				timer = ( TL += toAdd );
				if ( timer < prevTimer )
					TL += TH;
				break;
			case 3:
				return ERR_UNSUPPORTED_MODE;
		}
		if ( timer < prevTimer ) //Timer overflow
			Bits::setBit( TCON, TF, true );
	}
	return cycles;
}
