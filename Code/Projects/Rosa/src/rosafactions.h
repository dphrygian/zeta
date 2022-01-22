#ifndef ROSAFACTIONS_H
#define ROSAFACTIONS_H

class HashedString;

namespace RosaFactions
{
	enum EFactionCon
	{
		EFR_Hostile,
		EFR_Neutral,
		EFR_Friendly,
	};

	// For managing statically allocated memory
	void		AddRef();
	void		Release();

	EFactionCon	GetCon( const HashedString& FactionA, const HashedString& FactionB );
}

#endif // ROSAFACTIONS_H
