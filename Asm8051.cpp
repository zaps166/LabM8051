#include "Asm8051.hpp"
#include "HexFile.hpp"

#include <QStringList>
#include <QtEndian>
#include <QFile>

enum ALIAS { ALIAS_EQU, ALIAS_DATA, ALIAS_BIT, ALIAS_CODE };

typedef QHash< QString, QPair< QString, int > > Labels;
typedef QHash< QString, QPair< QString, ALIAS > > Aliases;

struct Line
{
	static bool memOffsetCmp( const Line &l1, const Line &l2 )
	{
		return l1.mem_offset < l2.mem_offset;
	}

	QString mnemonic;
	QStringList operands;
	quint16 mem_offset;
	bool ORGChanged;
	quint32 line;
	QByteArray compiled;
};

static inline quint8 getRegNumber( const QString &operand, bool &ok, quint8 max_reg_nr = 7 )
{
	quint8 reg_nr = 0xFF;
	if ( operand.length() == 2 )
		reg_nr = operand[ 1 ].digitValue();
	ok = reg_nr <= max_reg_nr;
	return reg_nr;
}

static inline quint16 getNumber( QString str, bool &ok )
{
	if ( str.isEmpty() || ( !str[ 0 ].isDigit() && str[ 0 ] != '-' ) )
		return ( ok = false );
	int base = 10;
	const QChar c = str.right( 1 )[ 0 ];
	if ( !c.isDigit() )
	{
		switch ( c.toLatin1() )
		{
			case 'B':
				base = 2;
				break;
			case 'O':
				base = 8;
				break;
			case 'H':
				base = 16;
				break;
			default:
				return ( ok = false );
		}
		str.chop( 1 );
	}
	return str.toInt( &ok, base );
}

static inline bool MOV_adr8_number8( const QString &operand, QByteArray &compiled, quint8 nr8_opcode, quint8 adr8_opcode )
{
	bool ok;
	if ( operand.left( 1 ) == "#" )
	{
		const quint8 number8 = getNumber( operand.mid( 1 ), ok );
		if ( ok )
		{
			compiled.append( nr8_opcode );
			compiled.append( number8 );
			return true;
		}
	}
	else
	{
		const quint8 adr8 = getNumber( operand, ok );
		if ( ok )
		{
			compiled.append( adr8_opcode );
			compiled.append( adr8 );
			return true;
		}
	}
	return false;
}

static inline bool IS_8_BIT( qint16 d )
{
	return d >= -128 && d <= 127;
}

static quint8 Compile( Line &l, bool &ov ) //Zwraca liczbę bajtów (jeżeli błąd, zwraca tylko jeżeli instrukcja może przyjąć jako parametr adres z LABELa)
{
	ov = false;
	if ( !l.compiled.isEmpty() )
		return l.compiled.size();
	bool ok = true, ok2;
	switch ( l.operands.size() )
	{
		case 0:
			if ( l.mnemonic == "NOP" )
				return l.compiled.append( ( char )0x00 ).size();
			else if ( l.mnemonic == "RET" )
				return l.compiled.append( 0x22 ).size();
			else if ( l.mnemonic == "RETI" )
				return l.compiled.append( 0x32 ).size();
			break;
		case 1:
		{
			const QString operand = l.operands[ 0 ];
			if ( operand != "A" && ( l.mnemonic == "SETB" || l.mnemonic == "CLR" || l.mnemonic == "CPL" ) )
			{
				const quint8 start_opcode = ( l.mnemonic == "SETB" ) ? 0xD2 : ( ( l.mnemonic == "CLR" ) ? 0xC2 : 0xB2 );
				if ( operand == "C" )
					return l.compiled.append( start_opcode + 0x01 ).size();
				else
				{
					const quint8 bit_code = getNumber( operand, ok );
					if ( ok )
					{
						l.compiled.append( start_opcode );
						return l.compiled.append( bit_code ).size();
					}
				}
			}
			else if ( l.mnemonic == "ACALL" || l.mnemonic == "AJMP" )
			{
				const quint16 adr11 = getNumber( operand, ok );
				if ( ok && adr11 + l.mem_offset <= 0x800 /*Powyżej przekroczony zakres 11bitów*/ )
				{
					l.compiled.append( ( ( adr11 >> 3 ) & 0xE0 ) | ( l.mnemonic == "ACALL" ? 0x11 : 0x01 ) );
					l.compiled.append( adr11 & 0xFF );
				}
				else
					ov = ok;
				return 2;
			}
			else if ( l.mnemonic == "SJMP" )
			{
				const qint16 d = getNumber( operand, ok ) - l.mem_offset - 2;
				if ( ok && IS_8_BIT( d ) )
				{
					l.compiled.append( 0x80 );
					l.compiled.append( d );
				}
				else
					ov = ok;
				return 2;
			}
			else if ( l.mnemonic == "LJMP" || l.mnemonic == "LCALL" )
			{
				const quint16 adr16 = qToBigEndian( getNumber( operand, ok ) );
				if ( ok )
				{
					l.compiled.append( l.mnemonic == "LCALL" ? 0x12 : 0x02 );
					l.compiled.append( ( const char * )&adr16, sizeof adr16 );
				}
				return 3;
			}
			else if ( l.mnemonic == "JMP" && operand == "@A+DPTR" )
				return l.compiled.append( 0x73 ).size();
			else if ( l.mnemonic == "JC" || l.mnemonic == "JNC" || l.mnemonic == "JZ" || l.mnemonic == "JNZ" )
			{
				const qint16 d = getNumber( operand, ok ) - l.mem_offset - 2;
				if ( ok && IS_8_BIT( d ) )
				{
					if ( l.mnemonic == "JC" )
						l.compiled.append( 0x40 );
					else if ( l.mnemonic == "JNC" )
						l.compiled.append( 0x50 );
					else if ( l.mnemonic == "JZ" )
						l.compiled.append( 0x60 );
					else if ( l.mnemonic == "JNZ" )
						l.compiled.append( 0x70 );
					l.compiled.append( d );
				}
				else
					ov = ok;
				return 2;
			}
			else if ( l.mnemonic == "INC" || l.mnemonic == "DEC" )
			{
				const quint8 start_opcode = ( l.mnemonic == "INC" ) ? 0x04 : 0x14;
				if ( operand == "A" )
					return l.compiled.append( start_opcode + 0x00 ).size();
				else if ( operand == "@R0" )
					return l.compiled.append( start_opcode + 0x02 ).size();
				else if ( operand == "@R1" )
					return l.compiled.append( start_opcode + 0x03 ).size();
				else if ( operand.left( 1 ) == "R" )
				{
					const quint8 reg_nr = getRegNumber( operand, ok );
					if ( ok )
						return l.compiled.append( start_opcode + 0x04 + reg_nr ).size();
				}
				else if ( operand == "DPTR" )
				{
					if ( start_opcode == 0x04 ) //DPTR tylko dla INC
						return l.compiled.append( 0xA3 ).size();
				}
				else
				{
					const quint8 adr8 = getNumber( operand, ok );
					if ( ok )
					{
						l.compiled.append( start_opcode + 0x01 );
						return l.compiled.append( adr8 ).size();
					}
				}
			}
			else if ( l.mnemonic == "PUSH" || l.mnemonic == "POP" )
			{
				const quint8 adr8 = getNumber( operand, ok );
				if ( ok )
				{
					l.compiled.append( l.mnemonic == "PUSH" ? 0xC0 : 0xD0 );
					return l.compiled.append( adr8 ).size();
				}
			}
			else if ( operand == "A" )
			{
				if ( l.mnemonic == "RR" )
					return l.compiled.append( 0x03 ).size();
				else if ( l.mnemonic == "RRC" )
					return l.compiled.append( 0x13 ).size();
				else if ( l.mnemonic == "RL" )
					return l.compiled.append( 0x23 ).size();
				else if ( l.mnemonic == "RLC" )
					return l.compiled.append( 0x33 ).size();
				else if ( l.mnemonic == "SWAP" )
					return l.compiled.append( 0xC4 ).size();
				else if ( l.mnemonic == "DA" )
					return l.compiled.append( 0xD4 ).size();
				else if ( l.mnemonic == "CLR" )
					return l.compiled.append( 0xE4 ).size();
				else if ( l.mnemonic == "CPL" )
					return l.compiled.append( 0xF4 ).size();
			}
			else if ( l.mnemonic == "DB" )
			{
				const quint8 byte = getNumber( l.operands[ 0 ], ok );
				if ( ok )
					l.compiled.append( byte );
				return 1;
			}
			else if ( l.mnemonic == "DW" )
			{
				const quint16 word = qToBigEndian( getNumber( l.operands[ 0 ], ok ) );
				if ( ok )
					l.compiled.append( ( const char * )&word, sizeof word );
				return 2;
			}
			else if ( operand == "AB" )
			{
				if ( l.mnemonic == "MUL" )
					return l.compiled.append( 0xA4 ).size();
				if ( l.mnemonic == "DIV" )
					return l.compiled.append( 0x84 ).size();
			}
		} break;
		case 2:
			if ( l.mnemonic == "MOV" )
			{
				if ( l.operands[ 0 ] == "A" )
				{
					if ( l.operands[ 1 ].left( 1 ) == "R" )
					{
						const quint8 reg_nr = getRegNumber( l.operands[ 1 ], ok );
						if ( ok )
							return l.compiled.append( 0xE8 + reg_nr ).size();
					}
					if ( l.operands[ 1 ] == "@R0" )
						return l.compiled.append( 0xE6 ).size();
					if ( l.operands[ 1 ] == "@R1" )
						return l.compiled.append( 0xE7 ).size();
					else
					{
						MOV_adr8_number8( l.operands[ 1 ], l.compiled, 0x74, 0xE5 );
						return 2;
					}
				}
				else if ( l.operands[ 0 ].left( 1 ) == "R" )
				{
					const quint8 reg_nr = getRegNumber( l.operands[ 0 ], ok );
					if ( ok )
					{
						if ( l.operands[ 1 ] == "A" )
							return l.compiled.append( 0xF8 + reg_nr ).size();
						else
						{
							MOV_adr8_number8( l.operands[ 1 ], l.compiled, 0x78 + reg_nr, 0xA8 + reg_nr );
							return 2;
						}
					}
				}
				else if ( l.operands[ 0 ].left( 2 ) == "@R" )
				{
					const quint8 reg_nr = getRegNumber( l.operands[ 0 ].mid( 1 ), ok, 1 );
					if ( ok )
					{
						if ( l.operands[ 1 ] == "A" )
							return l.compiled.append( 0xF6 + reg_nr ).size();
						else
						{
							MOV_adr8_number8( l.operands[ 1 ], l.compiled, 0x76 + reg_nr, 0xA6 + reg_nr );
							return 2;
						}
					}
				}
				else if ( l.operands[ 0 ] == "DPTR" && l.operands[ 1 ].left( 1 ) == "#" )
				{
					const quint16 number16 = qToBigEndian( getNumber( l.operands[ 1 ].mid( 1 ), ok ) );
					if ( ok )
					{
						l.compiled.append( 0x90 );
						l.compiled.append( ( const char * )&number16, sizeof number16 );
					}
					return 3;
				}
				else if ( l.operands[ 0 ] == "C" )
				{
					const quint8 bit_code = getNumber( l.operands[ 1 ], ok );
					if ( ok )
					{
						l.compiled.append( 0xA2 );
						return l.compiled.append( bit_code ).size();
					}
				}
				else if ( l.operands[ 1 ] == "C" )
				{
					const quint8 bit_code = getNumber( l.operands[ 0 ], ok );
					if ( ok )
					{
						l.compiled.append( 0x92 );
						return l.compiled.append( bit_code ).size();
					}
				}
				else
				{
					const quint8 adr8 = getNumber( l.operands[ 0 ], ok );
					if ( ok )
					{
						if ( l.operands[ 1 ] == "A" )
						{
							l.compiled.append( 0xF5 );
							return l.compiled.append( adr8 ).size();
						}
						else if ( l.operands[ 1 ].left( 1 ) == "R" )
						{
							const quint8 reg_nr = getRegNumber( l.operands[ 1 ], ok );
							if ( ok )
							{
								l.compiled.append( 0x88 + reg_nr );
								return l.compiled.append( adr8 ).size();
							}
						}
						else if ( l.operands[ 1 ] == "@R0" )
						{
							l.compiled.append( 0x86 );
							return l.compiled.append( adr8 ).size();
						}
						else if ( l.operands[ 1 ] == "@R1" )
						{
							l.compiled.append( 0x87 );
							return l.compiled.append( adr8 ).size();
						}
						else
						{
							if ( MOV_adr8_number8( l.operands[ 1 ], l.compiled, 0x75, 0x85 ) )
							{
								if ( ( quint8 )l.compiled[ 0 ] == 0x85 )
									l.compiled.append( adr8 );
								else
									l.compiled.insert( 1, adr8 );
								return l.compiled.size();
							}
							return 3;
						}
					}
				}
			}
			else if ( l.mnemonic == "MOVC" && l.operands[ 0 ] == "A" )
			{
				if ( l.operands[ 1 ] == "@A+PC" )
					return l.compiled.append( 0x83 ).size();
				else if ( l.operands[ 1 ] == "@A+DPTR" )
					return l.compiled.append( 0x93 ).size();
			}
			else if ( l.mnemonic == "MOVX" )
			{
				if ( l.operands[ 0 ] == "A" )
				{
					if ( l.operands[ 1 ] == "@R0" )
						return l.compiled.append( 0xE2 ).size();
					else if ( l.operands[ 1 ] == "@R1" )
						return l.compiled.append( 0xE3 ).size();
					else if ( l.operands[ 1 ] == "@DPTR" )
						return l.compiled.append( 0xE0 ).size();
				}
				else if ( l.operands[ 1 ] == "A" )
				{
					if ( l.operands[ 0 ] == "@R0" )
						return l.compiled.append( 0xF2 ).size();
					else if ( l.operands[ 0 ] == "@R1" )
						return l.compiled.append( 0xF3 ).size();
					else if ( l.operands[ 0 ] == "@DPTR" )
						return l.compiled.append( 0xF0 ).size();
				}
			}
			else if ( l.mnemonic == "JBC" || l.mnemonic == "JNB" || l.mnemonic == "JB" )
			{
				const qint16 d = getNumber( l.operands[ 1 ], ok ) - l.mem_offset - 3;
				const quint8 bit_code = getNumber( l.operands[ 0 ], ok2 );
				if ( ok && ok2 && IS_8_BIT( d ) )
				{
					l.compiled.append( l.mnemonic == "JBC" ? 0x10 : ( l.mnemonic == "JNB" ? 0x30 : 0x20 ) );
					l.compiled.append( bit_code );
					l.compiled.append( d );
				}
				else
					ov = ok && ok2;
				return 3;
			}
			else if ( l.mnemonic == "DJNZ" )
			{
				if ( l.operands[ 0 ].left( 1 ) == "R" )
				{
					const qint16 d = getNumber( l.operands[ 1 ], ok ) - l.mem_offset - 2;
					const quint8 reg_nr = getRegNumber( l.operands[ 0 ], ok2 );
					if ( ok && ok2 && IS_8_BIT( d ) )
					{
						l.compiled.append( 0xD8 + reg_nr );
						l.compiled.append( d );
					}
					else
						ov = ok && ok2;
					return 2;
				}
				else
				{
					const qint16 d = getNumber( l.operands[ 1 ], ok ) - l.mem_offset - 3;
					const quint8 adr8 = getNumber( l.operands[ 0 ], ok2 );
					if ( ok && ok2 && IS_8_BIT( d ) )
					{
						l.compiled.append( 0xD5 );
						l.compiled.append( adr8 );
						l.compiled.append( d );
					}
					else
						ov = ok && ok2;
					return 3;
				}
			}
			else if ( l.mnemonic == "XCH" && l.operands[ 0 ] == "A" )
			{
				if ( l.operands[ 1 ].left( 1 ) == "R" )
				{
					const quint8 reg_nr = getRegNumber( l.operands[ 1 ], ok );
					if ( ok )
						return l.compiled.append( 0xC8 + reg_nr ).size();
				}
				else if ( l.operands[ 1 ] == "@R0" )
					return l.compiled.append( 0xC6 ).size();
				else if ( l.operands[ 1 ] == "@R1" )
					return l.compiled.append( 0xC7 ).size();
				else
				{
					const quint8 adr8 = getNumber( l.operands[ 1 ], ok );
					if ( ok )
					{
						l.compiled.append( 0xC5 );
						return l.compiled.append( adr8 ).size();
					}
				}
			}
			else if ( l.mnemonic == "XCHD" && l.operands[ 0 ] == "A" )
			{
				if ( l.operands[ 1 ] == "@R0" )
					return l.compiled.append( 0xD6 ).size();
				else if ( l.operands[ 1 ] == "@R1" )
					return l.compiled.append( 0xD7 ).size();
			}
			else if ( l.mnemonic == "ADD" || l.mnemonic == "ADDC" || l.mnemonic == "SUBB" || l.mnemonic == "ANL" || l.mnemonic == "ORL" || l.mnemonic == "XRL" )
			{
				quint8 start_opcode = 0x00;
				QString operand = l.operands[ 1 ];
				if ( l.operands[ 0 ] == "C" && ( l.mnemonic == "ANL" || l.mnemonic == "ORL" ) )
				{
					if ( operand.left( 1 ) == "/" ) //negacja
					{
						operand.remove( 0, 1 );
						start_opcode = 0x2E;
					}
					const quint8 bit_code = getNumber( operand, ok );
					if ( ok )
					{
						if ( l.mnemonic == "ANL" )
							start_opcode += 0x82;
						else if ( l.mnemonic == "ORL" )
							start_opcode += 0x72;
						l.compiled.append( start_opcode );
						return l.compiled.append( bit_code ).size();
					}

				}
				else
				{
					if ( l.mnemonic == "ADD" )
						start_opcode = 0x24;
					else if ( l.mnemonic == "ADDC" )
						start_opcode = 0x34;
					else if ( l.mnemonic == "SUBB" )
						start_opcode = 0x94;
					else if ( l.mnemonic == "ANL" )
						start_opcode = 0x54;
					else if ( l.mnemonic == "ORL" )
						start_opcode = 0x44;
					else if ( l.mnemonic == "XRL" )
						start_opcode = 0x64;
					if ( l.operands[ 0 ] == "A" )
					{
						if ( operand.left( 1 ) == "R" )
						{
							const quint8 reg_nr = getRegNumber( operand, ok );
							if ( ok )
								return l.compiled.append( start_opcode + 4 + reg_nr ).size();
						}
						else if ( operand == "@R0" )
							return l.compiled.append( start_opcode + 2 ).size();
						else if ( operand == "@R1" )
							return l.compiled.append( start_opcode + 3 ).size();
						else if ( operand.left( 1 ) == "#" )
						{
							const quint8 number8 = getNumber( operand.mid( 1 ), ok );
							if ( ok )
							{
								l.compiled.append( start_opcode );
								l.compiled.append( number8 ).size();
							}
							return 2;
						}
						else
						{
							const quint8 adr8 = getNumber( operand, ok );
							if ( ok )
							{
								l.compiled.append( start_opcode + 1 );
								return l.compiled.append( adr8 ).size();
							}
						}
					}
					else if ( start_opcode == 0x64 || start_opcode == 0x54 || start_opcode == 0x44 ) //tylko ANL, XRL, ORL
					{
						const quint8 adr8 = getNumber( l.operands[ 0 ], ok );
						if ( ok )
						{
							start_opcode -= 2;
							if ( operand == "A" )
							{
								l.compiled.append( start_opcode );
								return l.compiled.append( adr8 ).size();
							}
							else if ( operand.left( 1 ) == "#" )
							{
								const quint8 number8 = getNumber( operand.mid( 1 ), ok );
								if ( ok )
								{
									l.compiled.append( start_opcode + 1 );
									l.compiled.append( adr8 );
									l.compiled.append( number8 ).size();
								}
								return 3;
							}
						}
					}
				}
			}
			break;
		case 3:
			if ( l.mnemonic == "CJNE" )
			{
				const bool isNumber = l.operands[ 1 ].left( 1 ) == "#";
				const quint8 number8 = getNumber( isNumber ? l.operands[ 1 ].mid( 1 ) : l.operands[ 1 ], ok2 );
				const qint16 d = getNumber( l.operands[ 2 ], ok ) - l.mem_offset - 3;
				if ( ok && ok2 && IS_8_BIT( d ) )
				{
					if ( l.operands[ 0 ] == "A" )
						l.compiled.append( 0xB4 + !isNumber );
					else if ( isNumber )
					{
						if ( l.operands[ 0 ].left( 1 ) == "R" )
						{
							const quint8 reg_nr = getRegNumber( l.operands[ 0 ], ok );
							if ( ok )
								l.compiled.append( 0xB8 + reg_nr );
						}
						else if ( l.operands[ 0 ] == "@R0" || l.operands[ 0 ] == "@R1" )
							l.compiled.append( 0xB6 + l.operands[ 0 ][ 2 ].digitValue() );
					}
					if ( l.compiled.size() == 1 )
					{
						l.compiled.append( number8 );
						l.compiled.append( d );
					}
				}
				else
					ov = ok && ok2;
				return 3;
			}
			break;
	}
	return 0;
}

static bool ParseAlias( const QString &name, const QString &line, Aliases &aliases )
{
	if ( !aliases.contains( name ) )
	{
		QStringList lineList = line.split( ' ' );
		if ( lineList.size() == 2 )
		{
			if ( lineList[ 0 ] == "EQU" )
				aliases[ name ].second = ALIAS_EQU;
			else if ( lineList[ 0 ] == "DATA" )
				aliases[ name ].second = ALIAS_DATA;
			else if ( lineList[ 0 ] == "BIT" )
				aliases[ name ].second = ALIAS_BIT;
			else if ( lineList[ 0 ] == "CODE" )
				aliases[ name ].second = ALIAS_CODE;
			else
				return false;
			aliases[ name ].first = lineList[ 1 ];
			return true;
		}
	}
	return false;
}

static void SimplifyCode( QString &line )
{
	const int commentIdx = line.indexOf( ';' );
	if ( commentIdx > -1 )
		line.remove( commentIdx, line.length() - commentIdx );
	line = line.simplified();
}

static void prependChar( QString &operand, const bool monkey, const bool hash )
{
	if ( monkey )
		operand.prepend( '@' );
	else if ( hash )
		operand.prepend( '#' );
}

/**/

QByteArray Asm8051::assemblyToHEX( QString fPth, ReturnStruct &ret, bool fillLinesInMemory )
{
	Labels labels;
	Aliases aliases;
	QList< Line > lines;

	QByteArray hex;

	quint16 ORG = 0x0000, currentByte = 0x0000;
	quint32 &currentLine = ret.line;
	bool ORGChanged = false, ov = false;

	class
	{
	public:
		inline quint16 getEndMemOffset() const
		{
			return mem_offset + compiled.size();
		}

		inline void addLine( QByteArray &hex )
		{
			hex += HexFile::toHex( HexFile::DataRecord, ( const quint8 * )compiled.data(), compiled.size(), mem_offset );
		}

		quint16 mem_offset;
		QByteArray compiled;
	} Hex = { 0x0000, QByteArray() };

	currentLine = 0;

	QFile f( fPth );
	if ( !f.open( QFile::ReadOnly | QFile::Text ) )
	{
		ret.err = FILE_ERROR;
		return QByteArray();
	}
	const QStringList src = QString( f.readAll() ).split( '\n' );
	f.close();

	const int nameIdx = fPth.lastIndexOf( '/' );
	if ( nameIdx > -1 )
		fPth.remove( nameIdx + 1, fPth.length() - nameIdx - 1 );
	else
		fPth.clear();

	++currentLine;
	ret.err = CODE_ERROR;

	foreach ( QString line, src ) //wczytuje INCLUDy, EQU, DATA, BIT, CODE, wstępnie wczytuje LABELe, sprawdza ORG oraz wczytuje wszystkie instrukcje
	{
		bool labelInLine = false;
		SimplifyCode( line );
		QString name = line.toUpper().mid( 0, line.indexOf( ' ' ) );
		if ( name == "$INCLUDE" )
		{
			QString incFPth = QString( line ).remove( 0, 8 ).remove( '(' ).remove( ')' ).simplified();
			if ( !QFile::exists( incFPth ) )
			{
				if ( QFile::exists( fPth + incFPth ) )
					incFPth.prepend( fPth );
				else if ( QFile::exists( ":/" + incFPth ) )
					incFPth.prepend( ":/" );
			}
			f.setFileName( incFPth );
			if ( !f.open( QFile::ReadOnly | QFile::Text ) )
				goto error;
			foreach ( QString inc_line, QString( f.readAll() ).split( '\n' ) )
			{
				SimplifyCode( inc_line );
				inc_line = inc_line.toUpper();
				const QString inc_name = inc_line.mid( 0, inc_line.indexOf( ' ' ) );
				inc_line.remove( 0, inc_name.length() + 1 );
				if ( !inc_line.isEmpty() )
					ParseAlias( inc_name, inc_line, aliases );
			}
			f.close();
		}
		else
		{
			line = line.toUpper();
			if ( name.right( 1 ) == ":" ) //LABEL
			{
				name.chop( 1 ); //wyrzucamy dwukropek
				//TODO: spr. czy label nie ma nazwy zastrzeżonej
				if ( labels.contains( name ) )
					goto error;
				labels[ name ] = qMakePair( QString(), lines.size() ); //dodajemy pustego LABELa
				line = line.remove( 0, name.length() + 1 ).simplified(); //wyrzucamy z linijki nazwę labela (może dalej być kod)
				name = line.mid( 0, line.indexOf( ' ' ) ); //ustalamy na nowo nazwę operacji
				labelInLine = true;
			}
			if ( name.right( 1 ) == ":" ) //nie może być już drugiego LABELa w linijce
				goto error;
			if ( !name.isEmpty() )
			{
				line.remove( 0, name.length() + 1 );
				if ( name == "ORG" )
				{
					bool ok;
					ORG = getNumber( line, ok );
					if ( !ok )
						goto error;
					ORGChanged = true;
				}
				else if ( ParseAlias( name, line, aliases ) )
				{
					if ( labelInLine )
						goto error;
				}
				else
				{
					Line l = { name, line.remove( ' ' ).split( ',', QString::SkipEmptyParts ), ORG, ORGChanged, currentLine, QByteArray() };
					if ( ( l.mnemonic == "PUSH" || l.mnemonic == "POP" ) && l.operands.count() == 1 && l.operands[ 0 ] == "A" && aliases.contains( "ACC" ) && aliases[ "ACC" ].second == ALIAS_DATA )
						l.operands[ 0 ] = aliases[ "ACC" ].first;
					lines += l;
					ORGChanged = false;
				}
			}
		}
		++currentLine;
	}

	for ( int i = 0 ; i < lines.size() ; ++i )
	{
		Line &l = lines[ i ];
		currentLine = l.line;
		if ( l.ORGChanged )
		{
			ORG = l.mem_offset;
			currentByte = 0x00;
		}
		else
			l.mem_offset += currentByte;
		//zamiana wartości z EQU, DATA, BIT i CODE oraz adresowanie bitowe
		for ( int j = 0 ; j < l.operands.size() ; ++j )
		{
			QString &operand = l.operands[ j ];
			if ( operand == "$" )
				operand = QString::number( l.mem_offset );
			else for ( int k = 0 ; k < 2 ; ++k )
			{
				if ( operand.contains( '.' ) ) //odwołanie się do bitu
				{
					QStringList operandL = operand.split( '.' );
					if ( operandL.size() == 2 )
					{
						bool ok;
						const quint8 bit_nr = getNumber( operandL[ 1 ], ok );
						if ( ok && bit_nr <= 7 )
						{
							bool negation = false;
							if ( ( negation = operandL[ 0 ].left( 1 ) == "/" ) )
								operandL[ 0 ].remove( 0, 1 );
							quint8 mem_addr;
							if ( aliases.contains( operandL[ 0 ] ) && aliases[ operandL[ 0 ] ].second != ALIAS_BIT )
								mem_addr = getNumber( aliases[ operandL[ 0 ] ].first, ok );
							else
								mem_addr = getNumber( operandL[ 0 ], ok );
							if ( ok )
							{
								if ( mem_addr & 0x80 )
								{
									if ( ( mem_addr >= 0x81 && mem_addr <= 0x87 ) || ( mem_addr >= 0x89 && mem_addr <= 0x8D ) || mem_addr == 0x99 )
									{
										ret.err = NON_BIT_ADDRESSABLE;
										goto error;
									}
									operand = QString::number( mem_addr + bit_nr );
								}
								else
								{
									if ( mem_addr < 0x20 || mem_addr > 0x2F )
									{
										ret.err = NON_BIT_ADDRESSABLE;
										goto error;
									}
									operand = QString::number( ( mem_addr - 0x20 ) * 8 + bit_nr );
								}
								if ( negation )
									operand.prepend( '/' );
								break;
							}
						}
					}
					goto error;
				}
				const bool monkey = operand.left( 1 ) == "@"; //:D
				const bool hash = operand.left( 1 ) == "#";
				if ( monkey || hash )
					operand.remove( 0, 1 );
				if ( aliases.contains( operand ) )
				{
					const ALIAS alias = aliases[ operand ].second;
					operand = aliases[ operand ].first;
					if ( alias == ALIAS_DATA || ( alias == ALIAS_BIT && !operand.contains( '.' ) ) )
					{
						prependChar( operand, monkey, hash );
						break;
					}
				}
				prependChar( operand, monkey, hash );

			}
		}
		for ( Labels::iterator it = labels.begin() ; it != labels.end() ; ++it ) //dopisywanie adresów do LABELi
			if ( it->second == i )
				it->first = QString::number( l.mem_offset );
		const quint8 b = Compile( l, ov ); //kompiluje to, co się da (oprócz tych co zawierają LABELe)
		if ( ov || !b )
			goto error;
		currentByte += b; //liczymy pozycję w bajtach, żeby np. ustawić adres LABELa lub pozycje w pliku HEX
	}

	for ( int i = 0 ; i < lines.size() ; ++i )
	{
		Line &l = lines[ i ];
		currentLine = l.line;
		for ( int j = 0 ; j < l.operands.size() ; ++j ) //zamiana LABELa na adres
		{
			QString &operand = l.operands[ j ];
			const bool hasHash = operand.left( 1 ) == "#";
			const QString tmpOperand = operand.mid( hasHash );
			if ( labels.contains( tmpOperand ) )
				operand = ( hasHash ? "#" : "" ) + labels[ tmpOperand ].first;
		}
		const quint8 b = Compile( l, ov ); //kompilacja pozostałych instrukcji
		if ( ov || !b || b != l.compiled.size() )
			goto error;
	}

	qStableSort( lines.begin(), lines.end(), Line::memOffsetCmp );

	for ( int i = 0 ; i < lines.size() ; ++i )
	{
		Line &l = lines[ i ];
		currentLine = l.line;
		if ( i ) //sprawdzenie, czy kody nie będą na siebie nachodzić (np. złe ustawienie ORG)
			if ( l.mem_offset + l.compiled.size() > lines[ i-1 ].mem_offset && lines[ i-1 ].mem_offset + lines[ i-1 ].compiled.size() > l.mem_offset )
			{
				ret.err = DATA_OVERWRITE_ERROR;
				goto error;
			}

		if ( Hex.compiled.size() <= 0x0D && Hex.getEndMemOffset() == l.mem_offset )
			Hex.compiled += l.compiled;
		else
		{
			Hex.addLine( hex );
			Hex.mem_offset = l.mem_offset;
			Hex.compiled = l.compiled;
		}
	}
	Hex.addLine( hex );

	ret.line = 0;
	ret.err = NO_ERR;
	if ( fillLinesInMemory )
		for ( int i = 0 ; i < lines.size() ; ++i )
			ret.linesInMemory.insert( lines[ i ].mem_offset, lines[ i ].line );
	return hex + HexFile::toHex( HexFile::EndOfFileRecord );
error:
	if ( ov )
		ret.err = JMP_TOO_FAR;
	ret.lineStr = src[ currentLine - 1 ].simplified();
	return QByteArray();
}
