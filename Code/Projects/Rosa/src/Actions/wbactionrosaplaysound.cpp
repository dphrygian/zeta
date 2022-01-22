#include "core.h"
#include "wbactionrosaplaysound.h"
#include "configmanager.h"
#include "wbactionstack.h"
#include "wbeventmanager.h"
#include "Components/wbcomprosatransform.h"

WBActionRosaPlaySound::WBActionRosaPlaySound()
:	m_EntityPE()
,	m_Sound()
,	m_SoundPE()
,	m_Attached( false )
,	m_Muted( false )
,	m_Volume( 0.0f )
,	m_VolumePE()
,	m_LocationPE()
{
}

WBActionRosaPlaySound::~WBActionRosaPlaySound()
{
}

/*virtual*/ void WBActionRosaPlaySound::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	WBAction::InitializeFromDefinition( DefinitionName );

	MAKEHASH( DefinitionName );

	STATICHASH( EntityPE );
	const SimpleString EntityPE = ConfigManager::GetString( sEntityPE, "", sDefinitionName );
	m_EntityPE.InitializeFromDefinition( EntityPE );

	STATICHASH( Sound );
	m_Sound = ConfigManager::GetHash( sSound, HashedString::NullString, sDefinitionName );

	STATICHASH( SoundPE );
	SimpleString SoundDef = ConfigManager::GetString( sSoundPE, "", sDefinitionName );
	m_SoundPE.InitializeFromDefinition( SoundDef );

	STATICHASH( Attached );
	m_Attached = ConfigManager::GetBool( sAttached, false, sDefinitionName );

	STATICHASH( Muted );
	m_Muted = ConfigManager::GetBool( sMuted, false, sDefinitionName );

	STATICHASH( Volume );
	m_Volume = ConfigManager::GetFloat( sVolume, 0.0f, sDefinitionName );

	STATICHASH( VolumePE );
	SimpleString VolumeDef = ConfigManager::GetString( sVolumePE, "", sDefinitionName );
	m_VolumePE.InitializeFromDefinition( VolumeDef );

	STATICHASH( LocationPE );
	SimpleString LocationDef = ConfigManager::GetString( sLocationPE, "", sDefinitionName );
	m_LocationPE.InitializeFromDefinition( LocationDef );
}

/*virtual*/ void WBActionRosaPlaySound::Execute()
{
	WBAction::Execute();

	WBEntity* const	pEntity = GetEntity();

	WBParamEvaluator::SPEContext PEContext;
	PEContext.m_Entity = pEntity;

	m_EntityPE.Evaluate( PEContext );
	WBEntity* const pSourceEntity = ( m_EntityPE.GetType() == WBParamEvaluator::EPT_Entity ) ? m_EntityPE.GetEntity() : pEntity;

	m_SoundPE.Evaluate( PEContext );
	const HashedString Sound = ( m_SoundPE.GetType() == WBParamEvaluator::EPT_String ) ? m_SoundPE.GetString() : m_Sound;

	m_VolumePE.Evaluate( PEContext );
	const float Volume = ( m_VolumePE.GetType() == WBParamEvaluator::EPT_Float ) ? m_VolumePE.GetFloat() : m_Volume;

	WBCompRosaTransform* const pTransform = pSourceEntity ? pSourceEntity->GetTransformComponent<WBCompRosaTransform>() : NULL;
	const Vector DefaultLocation = pTransform ? pTransform->GetLocation() : Vector();
	m_LocationPE.Evaluate( PEContext );
	const Vector Location = ( m_LocationPE.GetType() == WBParamEvaluator::EPT_Vector ) ? m_LocationPE.GetVector() : DefaultLocation;

	if( pSourceEntity )
	{
		WB_MAKE_EVENT( PlaySoundDef, pSourceEntity );
		WB_SET_AUTO( PlaySoundDef, Hash,	Sound,		Sound );
		WB_SET_AUTO( PlaySoundDef, Bool,	Attached,	m_Attached );
		WB_SET_AUTO( PlaySoundDef, Bool,	Muted,		m_Muted );
		WB_SET_AUTO( PlaySoundDef, Float,	Volume,		Volume );
		WB_SET_AUTO( PlaySoundDef, Vector,	Location,	Location );
		WB_DISPATCH_EVENT( GetEventManager(), PlaySoundDef, pSourceEntity );
	}
}
