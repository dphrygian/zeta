#ifndef ADDWBPEFACTORY

#include "PEs/wbpeconstantbool.h"
#include "PEs/wbpeconstantint.h"
#include "PEs/wbpeconstantfloat.h"
#include "PEs/wbpeconstantvector.h"
#include "PEs/wbpeconstantangles.h"
#include "PEs/wbpeconstantstring.h"
#include "PEs/wbperandombool.h"
#include "PEs/wbperandomint.h"
#include "PEs/wbperandomfloat.h"
#include "PEs/wbperandomvector.h"
#include "PEs/wbpeand.h"
#include "PEs/wbpeor.h"
#include "PEs/wbpexor.h"
#include "PEs/wbpenot.h"
#include "PEs/wbpeadd.h"
#include "PEs/wbpesub.h"
#include "PEs/wbpemul.h"
#include "PEs/wbpediv.h"
#include "PEs/wbpeneg.h"
#include "PEs/wbpeabs.h"
#include "PEs/wbpepow.h"
#include "PEs/wbpesquare.h"
#include "PEs/wbpemin.h"
#include "PEs/wbpemax.h"
#include "PEs/wbpedot.h"
#include "PEs/wbpecross.h"
#include "PEs/wbpequeryactionstack.h"
#include "PEs/wbpeowner.h"
#include "PEs/wbpeself.h"
#include "PEs/wbpeconditional.h"
#include "PEs/wbpestatmod.h"
#include "PEs/wbpegetname.h"
#include "PEs/wbpepushcontext.h"
#include "PEs/wbpelookup.h"
#include "PEs/wbpegetvariable.h"
#include "PEs/wbpegetentitybylabel.h"
#include "PEs/wbpeselector.h"
#include "PEs/wbpenormal.h"
#include "PEs/wbpegetstate.h"
#include "PEs/wbpenull.h"
#include "PEs/wbpesaturate.h"
#include "PEs/wbpelength.h"
#include "PEs/wbpeisstate.h"
#include "PEs/wbpegetconfigvar.h"

#else

ADDWBPEFACTORY( ConstantBool );
ADDWBPEFACTORY( ConstantInt );
ADDWBPEFACTORY( ConstantFloat );
ADDWBPEFACTORY( ConstantVector );
ADDWBPEFACTORY( ConstantAngles );
ADDWBPEFACTORY( ConstantString );
ADDWBPEFACTORY( RandomBool );
ADDWBPEFACTORY( RandomInt );
ADDWBPEFACTORY( RandomFloat );
ADDWBPEFACTORY( RandomVector );
ADDWBPEFACTORY( AND );
ADDWBPEFACTORY( OR );
ADDWBPEFACTORY( XOR );
ADDWBPEFACTORY( NOT );
ADDWBPEFACTORY( Add );
ADDWBPEFACTORY( Sub );
ADDWBPEFACTORY( Mul );
ADDWBPEFACTORY( Div );
ADDWBPEFACTORY( Neg );
ADDWBPEFACTORY( Abs );
ADDWBPEFACTORY( Pow );
ADDWBPEFACTORY( Square );
ADDWBPEFACTORY( Min );
ADDWBPEFACTORY( Max );
ADDWBPEFACTORY( Dot );
ADDWBPEFACTORY( Cross );
ADDWBPEFACTORY( QueryActionStack );
ADDWBPEFACTORY( Owner );
ADDWBPEFACTORY( Self );
ADDWBPEFACTORY( Conditional );
ADDWBPEFACTORY( StatMod );
ADDWBPEFACTORY( GetName );
ADDWBPEFACTORY( PushContext );
ADDWBPEFACTORY( Lookup );
ADDWBPEFACTORY( GetVariable );
ADDWBPEFACTORY( GetEntityByLabel );
ADDWBPEFACTORY( Selector );
ADDWBPEFACTORY( Normal );
ADDWBPEFACTORY( GetState );
ADDWBPEFACTORY( Null );
ADDWBPEFACTORY( Saturate );
ADDWBPEFACTORY( Length );
ADDWBPEFACTORY( IsState );
ADDWBPEFACTORY( GetConfigVar );

#endif
