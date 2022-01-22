#include "core.h"
#include "wbactionrosasetlightcolor.h"
#include "wbeventmanager.h"
#include "configmanager.h"

WBActionRosaSetLightColor::WBActionRosaSetLightColor()
:	m_ColorH( 0.0f )
,	m_ColorS( 0.0f )
,	m_ColorV( 0.0f )
{
}

WBActionRosaSetLightColor::~WBActionRosaSetLightColor()
{
}

/*virtual*/ void WBActionRosaSetLightColor::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( ColorH );
	m_ColorH = ConfigManager::GetFloat( sColorH, 0.0f, sDefinitionName );

	STATICHASH( ColorS );
	m_ColorS = ConfigManager::GetFloat( sColorS, 0.0f, sDefinitionName );

	STATICHASH( ColorV );
	m_ColorV = ConfigManager::GetFloat( sColorV, 0.0f, sDefinitionName );
}

/*virtual*/ void WBActionRosaSetLightColor::Execute()
{
	WBAction::Execute();

	WBEntity* const pEntity = GetEntity();
	if( pEntity )
	{
		WB_MAKE_EVENT( SetLightColorHSV, pEntity );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorH, m_ColorH );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorS, m_ColorS );
		WB_SET_AUTO( SetLightColorHSV, Float, ColorV, m_ColorV );
		WB_DISPATCH_EVENT( GetEventManager(), SetLightColorHSV, pEntity );
	}
}
