// For components declared in external libraries, make another header of this form
// and include it where needed to register these names.

// #define ADDWBCOMPONENT( type ) before including this file, then include it inline
// to handle every component type as needed.

#ifndef ADDWBCOMPONENT

#include "Components/wbcomptransform.h"
#include "Components/wbcompowner.h"
#include "Components/wbcompreactions.h"
#include "Components/wbcompstatmod.h"
#include "Components/wbcompstate.h"
#include "Components/wbcomppemap.h"
#include "Components/wbcompvariablemap.h"
#include "Components/wbcomplabel.h"

#else

ADDWBCOMPONENT( Transform );
ADDWBCOMPONENT( Owner );		// ROSANOTE: Moved before Reactions so owner will be set before a reaction may need it
ADDWBCOMPONENT( Reactions );
ADDWBCOMPONENT( StatMod );
ADDWBCOMPONENT( State );
ADDWBCOMPONENT( PEMap );
ADDWBCOMPONENT( VariableMap );
ADDWBCOMPONENT( Label );

#endif
