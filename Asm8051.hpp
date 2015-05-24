#ifndef ASM8051_HPP
#define ASM8051_HPP

#include <QByteArray>
#include <QString>
#include <QHash>

namespace Asm8051
{
	class LinesInMemory : public QHash< quint16, quint32 >
	{
	public:
		inline quint32 getLine( quint16 memory_offset ) const
		{
			if ( contains( memory_offset ) )
				return value( memory_offset );
			return 0;
		}
	};

	enum ERROR { FILE_ERROR = -5, NON_BIT_ADDRESSABLE, JMP_TOO_FAR, CODE_ERROR, DATA_OVERWRITE_ERROR, NO_ERR };

	struct ReturnStruct
	{
		ERROR err;
		quint32 line;
		QString lineStr;
		LinesInMemory linesInMemory;
	};

	QByteArray assemblyToHEX( QString fPth, ReturnStruct &ret, bool fillLinesInMemory );
}

#endif // ASM8051_HPP
