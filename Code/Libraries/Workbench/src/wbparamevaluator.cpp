#include "core.h"
#include "wbparamevaluator.h"
#include "simplestring.h"
#include "wbpe.h"
#include "wbparamevaluatorfactory.h"
#include "reversehash.h"

WBParamEvaluator::SEvaluatedParam::SEvaluatedParam()
:	m_Type( EPT_None )
,	m_Bool( false )
,	m_Int( 0 )
,	m_Float( 0.0f )
,	m_String( "" )
,	m_Entity()
,	m_Vector()
,	m_Angles()
{
}

WBParamEvaluator::SPEContext::SPEContext()
:	m_Entity( NULL )
{
}

WBParamEvaluator::WBParamEvaluator()
:	m_RootEvaluator( NULL )
{
}

WBParamEvaluator::~WBParamEvaluator()
{
	SafeDelete( m_RootEvaluator );
}

void WBParamEvaluator::InitializeFromDefinition( const SimpleString& DefinitionName )
{
	// Delete the root if we have one; this way, reinitializing from an empty definition will delete the evaluator tree as expected.
	SafeDelete( m_RootEvaluator );

	// Initialize evaluated param (again, for correct behavior when reinitializing)
	m_EvaluatedParam.Initialize();

	if( DefinitionName == "" )
	{
		// Gracefully handle undefined PEs (used for pattern matching without a max, for instance).
		return;
	}
	else
	{
		// Just forward this definition to the root, we don't actually need to configure anything here
		m_RootEvaluator = WBParamEvaluatorFactory::Create( DefinitionName );
	}
}

void WBParamEvaluator::Evaluate( const SPEContext& Context )
{
	// 19 Feb 2016: Reinitializing this so that a nonevaluable PE will no longer return stale results.
	// Can't imagine this will cause problems, but I'm mentioning it just in case.
	m_EvaluatedParam.Initialize();

	if( m_RootEvaluator )
	{
		m_RootEvaluator->Evaluate( Context, m_EvaluatedParam );
	}
}

void WBParamEvaluator::SEvaluatedParam::Initialize()
{
	m_Type = EPT_None;
}

bool WBParamEvaluator::SEvaluatedParam::GetBool() const
{
	switch( m_Type )
	{
	case WBParamEvaluator::EPT_Bool:	return m_Bool;
	case WBParamEvaluator::EPT_Int:		return ( m_Int != 0 );
	case WBParamEvaluator::EPT_Float:	return ( m_Float != 0.0f );
	case WBParamEvaluator::EPT_Entity:	return ( m_Entity.Get() != NULL );
	case WBParamEvaluator::EPT_String:	return ( m_String != "" );
	default:							return false;
	}
}

int WBParamEvaluator::SEvaluatedParam::GetInt() const
{
	return m_Type == WBParamEvaluator::EPT_Int ? m_Int : m_Type == WBParamEvaluator::EPT_Float ? static_cast<int>( m_Float ) : 0;
}

float WBParamEvaluator::SEvaluatedParam::GetFloat() const
{
	return m_Type == WBParamEvaluator::EPT_Float ? m_Float : m_Type == WBParamEvaluator::EPT_Int ? static_cast<float>( m_Int ) : 0.0f;
}

SimpleString WBParamEvaluator::SEvaluatedParam::GetString() const
{
	return m_Type == WBParamEvaluator::EPT_String ? m_String : SimpleString( "" );
}

WBEntity* WBParamEvaluator::SEvaluatedParam::GetEntity() const
{
	return m_Type == WBParamEvaluator::EPT_Entity ? m_Entity.Get() : NULL;
}

Vector WBParamEvaluator::SEvaluatedParam::GetVector() const
{
	return m_Type == WBParamEvaluator::EPT_Vector ? m_Vector : m_Type == WBParamEvaluator::EPT_Angles ? m_Angles.ToVector() : Vector();
}

Angles WBParamEvaluator::SEvaluatedParam::GetAngles() const
{
	return m_Type == WBParamEvaluator::EPT_Angles ? m_Angles : m_Type == WBParamEvaluator::EPT_Vector ? m_Vector.ToAngles() : Angles();
}

void WBParamEvaluator::SEvaluatedParam::Set( const SEvaluatedParam& EvaluatedParam )
{
	switch( EvaluatedParam.m_Type )
	{
	case EPT_None:		m_Type = EPT_None;													break;
	case EPT_Bool:		m_Type = EPT_Bool;		m_Bool		= EvaluatedParam.GetBool();		break;
	case EPT_Int:		m_Type = EPT_Int;		m_Int		= EvaluatedParam.GetInt();		break;
	case EPT_Float:		m_Type = EPT_Float;		m_Float		= EvaluatedParam.GetFloat();	break;
	case EPT_String:	m_Type = EPT_String;	m_String	= EvaluatedParam.GetString();	break;
	case EPT_Entity:	m_Type = EPT_Entity;	m_Entity	= EvaluatedParam.GetEntity();	break;
	case EPT_Vector:	m_Type = EPT_Vector;	m_Vector	= EvaluatedParam.GetVector();	break;
	case EPT_Angles:	m_Type = EPT_Angles;	m_Angles	= EvaluatedParam.GetAngles();	break;
	default:																				break;
	}
}

void WBParamEvaluator::SEvaluatedParam::Set( const WBParamEvaluator& ParamEvaluator )
{
	switch( ParamEvaluator.GetType() )
	{
	case EPT_None:		m_Type = EPT_None;													break;
	case EPT_Bool:		m_Type = EPT_Bool;		m_Bool		= ParamEvaluator.GetBool();		break;
	case EPT_Int:		m_Type = EPT_Int;		m_Int		= ParamEvaluator.GetInt();		break;
	case EPT_Float:		m_Type = EPT_Float;		m_Float		= ParamEvaluator.GetFloat();	break;
	case EPT_String:	m_Type = EPT_String;	m_String	= ParamEvaluator.GetString();	break;
	case EPT_Entity:	m_Type = EPT_Entity;	m_Entity	= ParamEvaluator.GetEntity();	break;
	case EPT_Vector:	m_Type = EPT_Vector;	m_Vector	= ParamEvaluator.GetVector();	break;
	case EPT_Angles:	m_Type = EPT_Angles;	m_Angles	= ParamEvaluator.GetAngles();	break;
	default:																				break;
	}
}

void WBParamEvaluator::SEvaluatedParam::Set( const WBEvent::SParameter* const pParameter )
{
	if( !pParameter )
	{
		return;
	}

	switch( pParameter->GetType() )
	{
	case WBEvent::EWBEPT_None:		m_Type = EPT_None;																			break;
	case WBEvent::EWBEPT_Bool:		m_Type = EPT_Bool;		m_Bool		= pParameter->GetBool();								break;
	case WBEvent::EWBEPT_Int:		m_Type = EPT_Int;		m_Int		= pParameter->GetInt();									break;
	case WBEvent::EWBEPT_Float:		m_Type = EPT_Float;		m_Float		= pParameter->GetFloat();								break;
	case WBEvent::EWBEPT_Hash:		m_Type = EPT_String;	m_String	= ReverseHash::ReversedHash( pParameter->GetHash() );	break;
	case WBEvent::EWBEPT_Vector:	m_Type = EPT_Vector;	m_Vector	= pParameter->GetVector();								break;
	case WBEvent::EWBEPT_Angles:	m_Type = EPT_Angles;	m_Angles	= pParameter->GetAngles();								break;
	case WBEvent::EWBEPT_Entity:	m_Type = EPT_Entity;	m_Entity	= pParameter->GetEntity();								break;
	case WBEvent::EWBEPT_Pointer:	WARN;																						break;
	default:																													break;
	}
}

// If both values are ints, or one is an int and the other is untyped, it's an int op.
/*static*/ bool WBParamEvaluator::IsIntOp( const WBParamEvaluator::SEvaluatedParam& ValueA, const WBParamEvaluator::SEvaluatedParam& ValueB )
{
	return
		( ValueA.m_Type == EPT_Int && ValueB.m_Type == EPT_Int ) ||
		( ValueA.m_Type == EPT_Int && ValueB.m_Type == EPT_None ) ||
		( ValueA.m_Type == EPT_None && ValueB.m_Type == EPT_Int );
}
