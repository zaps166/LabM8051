#ifndef FUNCTIONS_HPP
#define FUNCTIONS_HPP

#include <QString>
#include <QFont>

namespace Functions
{
	static inline QString toBin( quint8 i )
	{
		return QString( "%1" ).arg( i, 8, 2, QChar( '0' ) );
	}
	static inline QString toHex( quint8 i )
	{
		return "0x" + QString( "%1" ).arg( i, 2, 16, QChar( '0' ) ).toUpper();
	}
	static inline QString toHex( quint16 i )
	{
		return "0x" + QString( "%1" ).arg( i, 4, 16, QChar( '0' ) ).toUpper();
	}

	static inline QFont getMonospaceFont( QFont f = QFont() )
	{
#ifndef Q_OS_WIN
		f.setFamily( "Monospace" );
#else
		f.setFamily( "Courier" );
#endif
		f.setStyleStrategy( QFont::PreferAntialias );
		return f;
	}
}

#endif // FUNCTIONS_HPP
