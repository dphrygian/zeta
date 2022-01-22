#ifndef REVERSEHASH_H
#define REVERSEHASH_H

class HashedString;
class SimpleString;

namespace ReverseHash
{
	void			Initialize();
	void			ShutDown();

	bool			IsEnabled();

	void			RegisterHash( const HashedString& Hash, const SimpleString& String );
	bool			IsRegistered( const HashedString& Hash );
	SimpleString	ReversedHash( const HashedString& Hash );

	void			ReportSize();
}

#if BUILD_DEV
// For viewing in watch window
const char* ReversedHash( const HashedString& Hash );
const char* ReversedHash( const uint Hash );
#endif

#endif
