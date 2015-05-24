#include "HexFile.hpp"

#include <QtEndian>
#include <QBuffer>

HexFile::HexERROR HexFile::parse( const char *hex, uint32_t size, uint8_t *dest, uint32_t destSize )
{
	QByteArray hexArr = QByteArray::fromRawData( hex, size );
	QBuffer buff( &hexArr );
	if ( buff.open( QBuffer::ReadOnly | QBuffer::Text ) )
		while ( !buff.atEnd() )
		{
			QByteArray line = buff.readLine();
			if ( line.length() < 11 || line[ 0 ] != ':' )
				break;
			line = line.fromHex( line.mid( 1 ) );
			const quint8 byteCount = line[ 0 ];
			if ( line.length() != byteCount + 5 )
				break;
			const quint16 address = qFromBigEndian( *( quint16 * )( line.data() + 1 ) );
			const RecordType recordType = ( RecordType )( char )line[ 3 ];
			const QByteArray data = line.mid( 4, byteCount );
			const quint8 checksum = line[ 4 + byteCount ];
			quint8 checksum2 = 0;
			for ( int i = 0 ; i < line.length() - 1 ; ++i )
				checksum2 += ( quint8 )line[ i ];
			checksum2 = 0x100 - checksum2;
			if ( checksum != checksum2 )
				break;
			switch ( recordType )
			{
				case DataRecord:
					if ( ( unsigned )data.length() + address > destSize )
						return OUT_OF_ROM_MEMORY;
					memcpy( dest + address, data.data(), data.length() );
					break;
				case EndOfFileRecord:
					return NO_ERR;
				case ExtendedSegmentAddressRecord:
				case StartSegmentAddressRecord:
				case ExtendedLinearAddressRecord:
				case StartLinearAddressRecord:
					return UNSUPPORTED_RECORD_TYPE;
				default:
					return INVALID_RECORD_TYPE;
			}
		}
	return FILE_ERROR;
}
QByteArray HexFile::toHex( RecordType type, const uint8_t *code, uint8_t size, uint16_t offset )
{
	switch ( type )
	{
		case DataRecord:
		{
			QByteArray hex;
			if ( size )
			{
				offset = qToBigEndian( offset );
				QByteArray binary;
				binary.reserve( size + 5 );
				binary.append( size );
				binary.append( ( const char * )&offset, sizeof offset );
				binary.append( '\000' );
				binary.append( ( const char * )code, size );
				binary.append( '\000' );
				const int checksumPos = binary.size() - 1;
				for ( int i = 0 ; i < binary.size() - 1 ; ++i )
					binary[ checksumPos ] = binary[ checksumPos ] + ( quint8 )binary[ i ];
				binary[ checksumPos ] = 0x100 - binary[ checksumPos ];
				hex.reserve( binary.size() * 2 + 2 );
				hex.append( ':' );
				for ( int i = 0 ; i < binary.size() ; ++i )
					hex.append( QString( "%1" ).arg( ( quint8 )binary[ i ], 2, 16, QChar( '0' ) ).toUpper() );
				hex.append( '\n' );
			}
			return hex;
		}
		case EndOfFileRecord:
			return ":00000001FF\n";
		default:
			return QByteArray();
	}
}
