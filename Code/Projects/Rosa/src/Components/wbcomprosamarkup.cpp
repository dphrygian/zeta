#include "core.h"
#include "wbcomprosamarkup.h"
#include "configmanager.h"

WBCompRosaMarkup::WBCompRosaMarkup()
{
}

WBCompRosaMarkup::~WBCompRosaMarkup()
{
}

/*virtual*/ void WBCompRosaMarkup::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	Super::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Markup );
	m_Markup = ConfigManager::GetInheritedHash( sMarkup, HashedString::NullString, sDefinitionName );
}
