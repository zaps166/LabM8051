#ifndef HEXFILE_HPP
#define HEXFILE_HPP

#include <QByteArray>
#include <stdint.h>

namespace HexFile
{
	enum HexERROR { FILE_ERROR = -4, INVALID_RECORD_TYPE, UNSUPPORTED_RECORD_TYPE, OUT_OF_ROM_MEMORY, NO_ERR };
	enum RecordType
	{
		DataRecord,
		EndOfFileRecord,
		ExtendedSegmentAddressRecord,
		StartSegmentAddressRecord,
		ExtendedLinearAddressRecord,
		StartLinearAddressRecord
	};

	HexERROR parse( const char *hex, uint32_t size, uint8_t *dest, uint32_t destSize );
	QByteArray toHex( RecordType type, const uint8_t *code = NULL, uint8_t size = 0, uint16_t offset = 0 );
}

#endif // HEXFILE_HPP
