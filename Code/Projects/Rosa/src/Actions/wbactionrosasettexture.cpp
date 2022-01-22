#include "core.h"
#include "wbactionrosasettexture.h"
#include "wbeventmanager.h"
#include "configmanager.h"

WBActionRosaSetTexture::WBActionRosaSetTexture()
:	m_Texture()
{
}

WBActionRosaSetTexture::~WBActionRosaSetTexture()
{
}

/*virtual*/ void WBActionRosaSetTexture::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( Texture );
	m_Texture = ConfigManager::GetHash( sTexture, HashedString::NullString, sDefinitionName );
}

/*virtual*/ void WBActionRosaSetTexture::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();
	if( pEntity )
	{
		WB_MAKE_EVENT( SetTexture, pEntity );
		WB_SET_AUTO( SetTexture, Hash, Texture, m_Texture );
		WB_DISPATCH_EVENT( GetEventManager(), SetTexture, pEntity );
	}
}
