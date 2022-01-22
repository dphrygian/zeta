#ifndef WBCOMPPEMAP_H
#define WBCOMPPEMAP_H

#include "wbcomponent.h"
#include "wbpe.h"
#include "map.h"

class WBCompPEMap : public WBComponent
{
public:
	WBCompPEMap();
	virtual ~WBCompPEMap();

	DEFINE_WBCOMP( PEMap, WBComponent );

	virtual int		GetTickOrder() { return ETO_NoTick; }

	WBPE*			GetPE( const HashedString& Name ) const;

	// ROSANOTE: Made public so e.g. character component can attach PEs
	void			AddPEMap( const SimpleString& DefinitionName );

protected:
	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );

private:
	typedef Map<HashedString, WBPE*> TPEMap;
	TPEMap			m_PEMap;
};

#endif // WBCOMPPEMAP_H
