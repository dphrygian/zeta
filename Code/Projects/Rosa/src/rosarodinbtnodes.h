#ifndef ADDRODINBTNODEFACTORY

#include "BTNodes/rodinbtnoderosamoveto.h"
#include "BTNodes/rodinbtnoderosaturntoward.h"
#include "BTNodes/rodinbtnoderosastopmoving.h"
#include "BTNodes/rodinbtnoderosalookat.h"
#include "BTNodes/rodinbtnoderosaplayanim.h"
#include "BTNodes/rodinbtnoderosaplaybark.h"
#include "BTNodes/rodinbtnoderosachecktrace.h"

#else

ADDRODINBTNODEFACTORY( RosaMoveTo );
ADDRODINBTNODEFACTORY( RosaTurnToward );
ADDRODINBTNODEFACTORY( RosaStopMoving );
ADDRODINBTNODEFACTORY( RosaLookAt );
ADDRODINBTNODEFACTORY( RosaPlayAnim );
ADDRODINBTNODEFACTORY( RosaPlayBark );
ADDRODINBTNODEFACTORY( RosaCheckTrace );

#endif
