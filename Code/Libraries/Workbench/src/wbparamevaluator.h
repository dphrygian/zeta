#ifndef WBPARAMEVALUATOR_H
#define WBPARAMEVALUATOR_H

#include "simplestring.h"
#include "vector.h"
#include "angles.h"
#include "wbentityref.h"
#include "wbevent.h"

class WBPE;

class WBParamEvaluator
{
public:
	WBParamEvaluator();
	~WBParamEvaluator();

	struct SPEContext
	{
		SPEContext();

		WBEntity*	m_Entity;
	};

	enum EParamType
	{
		EPT_None,
		EPT_Bool,
		EPT_Int,
		EPT_Float,
		EPT_String,
		EPT_Entity,
		EPT_Vector,
		EPT_Angles,
	};

	void			InitializeFromDefinition( const SimpleString& DefinitionName );
	void			Evaluate( const SPEContext& Context );

	bool			IsInitialized() const	{ return m_RootEvaluator != NULL; }
	bool			HasRoot() const			{ return m_RootEvaluator != NULL; }
	bool			IsEvaluated() const		{ return m_EvaluatedParam.m_Type != EPT_None; }	// DLP 22 Aug 2016: Prefer this to HasRoot(), it works when a Lookup fails

	bool			GetBool() const		{ return m_EvaluatedParam.GetBool(); }
	int				GetInt() const		{ return m_EvaluatedParam.GetInt(); }
	float			GetFloat() const	{ return m_EvaluatedParam.GetFloat(); }
	SimpleString	GetString() const	{ return m_EvaluatedParam.GetString(); }
	WBEntity*		GetEntity() const	{ return m_EvaluatedParam.GetEntity(); }
	Vector			GetVector() const	{ return m_EvaluatedParam.GetVector(); }
	Angles			GetAngles() const	{ return m_EvaluatedParam.GetAngles(); }

	EParamType		GetType() const		{ return m_EvaluatedParam.m_Type; };

	struct SEvaluatedParam
	{
		SEvaluatedParam();

		EParamType		m_Type;

		// WBTODO: If I were concerned about memory, I could union these values (like WBEvents do)
		bool			m_Bool;
		int				m_Int;
		float			m_Float;
		SimpleString	m_String;
		WBEntityRef		m_Entity;
		Vector			m_Vector;
		Angles			m_Angles;

		void			Initialize();

		bool			GetBool() const;
		int				GetInt() const;
		float			GetFloat() const;
		SimpleString	GetString() const;
		WBEntity*		GetEntity() const;
		Vector			GetVector() const;
		Angles			GetAngles() const;

		// Urgh.
		void			Set( const SEvaluatedParam& EvaluatedParam );
		void			Set( const WBParamEvaluator& ParamEvaluator );
		void			Set( const WBEvent::SParameter* const pParameter );
	};

	static bool		IsIntOp( const WBParamEvaluator::SEvaluatedParam& ValueA, const WBParamEvaluator::SEvaluatedParam& ValueB );

private:
	WBPE*			m_RootEvaluator;
	SEvaluatedParam	m_EvaluatedParam;
};

#endif // WBPARAMEVALUATOR_H
