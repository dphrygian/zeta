#ifndef WBACTIONROSASELECTCHARACTERPRESET_H
#define WBACTIONROSASELECTCHARACTERPRESET_H

#include "wbaction.h"
#include "hashedstring.h"

class WBActionRosaSelectCharacterPreset : public WBAction
{
public:
	WBActionRosaSelectCharacterPreset();
	virtual ~WBActionRosaSelectCharacterPreset();

	DEFINE_WBACTION_FACTORY( RosaSelectCharacterPreset );

	virtual void	InitializeFromDefinition( const SimpleString& DefinitionName );
	virtual void	Execute();

private:
	int	m_SkinPreset;	// -1 means ignore this field
	int	m_NailsPreset;	// -1 means ignore this field
};

#endif // WBACTIONROSASELECTCHARACTERPRESET_H
