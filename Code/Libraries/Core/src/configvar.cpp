#include "core.h"
#include "configvar.h"

ConfigVar::ConfigVar()
:	m_Type( EVT_None )
,	m_String( NULL )	// Initialize this for the whole union; it's a pointer so it's bigger than the rest on x64
,	m_Hash( (unsigned int)0 )
#if PARANOID_HASH_CHECK
,	m_Name( "" )
#endif
{
}
