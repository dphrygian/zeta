#ifndef ADDWBCOMPONENT

#include "Components/wbcomprosacharacterconfig.h"
#include "Components/wbcomprosatransform.h"
#include "Components/wbcomprosaupgrades.h"
#include "Components/wbcomprosacollision.h"
#include "Components/wbcomprosaplayer.h"
#include "Components/wbcomprosamesh.h"
#include "Components/wbcomprosacharacter.h"
#include "Components/wbcomprosalight.h"
#include "Components/wbcomprosaantilight.h"
#include "Components/wbcomprosadecal.h"
#include "Components/wbcomprosatrapbolt.h"
#include "Components/wbcomprosahealth.h"
#include "Components/wbcomprosaparticles.h"
#include "Components/wbcomprosacamera.h"
#include "Components/wbcomprosafrobber.h"
#include "Components/wbcomprosafrobbable.h"
#include "Components/wbcomprosadoor.h"
#include "Components/wbcomprosainventory.h"
#include "Components/wbcomprosapickup.h"
#include "Components/wbcomprosaitem.h"
#include "Components/wbcomprosaweapon.h"
#include "Components/wbcomprosarope.h"
#include "Components/wbcomprosahands.h"
#include "Components/wbcomprosavisible.h"
#include "Components/wbcomprosasensorvision.h"
#include "Components/wbcomprosasensorhearing.h"
#include "Components/wbcomprosasensormarkup.h"
#include "Components/wbcomprosasensordamage.h"
#include "Components/wbcomprosasensortheft.h"
#include "Components/wbcomprosathinkertarget.h"
#include "Components/wbcomprosathinkerpatrol.h"
#include "Components/wbcomprosaaimotion.h"
#include "Components/wbcomprosafaction.h"
#include "Components/wbcomprosaheadtracker.h"
#include "Components/wbcomprosaragdoll.h"
#include "Components/wbcomprosahitbox.h"
#include "Components/wbcomprosamarkup.h"
#include "Components/wbcomprosasound.h"
#include "Components/wbcomprosafootsteps.h"
#include "Components/wbcomprosawallet.h"
#include "Components/wbcomprosaammobag.h"
#include "Components/wbcomprosalock.h"
#include "Components/wbcomprosarespawner.h"
#include "Components/wbcomprosaanchor.h"
#include "Components/wbcomprosaclimbable.h"
#include "Components/wbcomprosahudmarker.h"
#include "Components/wbcomprosapowerteleport.h"
#include "Components/wbcomprosakeyring.h"
#include "Components/wbcomprosamedkit.h"
#include "Components/wbcomprosasleeper.h"
#include "Components/wbcomprosaspikes.h"
#include "Components/wbcomprosaicicles.h"
#include "Components/wbcomprosapatrolpath.h"
#include "Components/wbcomprosasensorpatrol.h"
#include "Components/wbcomprosahackable.h"
#include "Components/wbcomprosasensorbump.h"
#include "Components/wbcomprosarepulsor.h"
#include "Components/wbcomprosaspin.h"
#include "Components/wbcomprosaswitch.h"
#include "Components/wbcomprosaswitchable.h"
#include "Components/wbcomprosaseccam.h"
#include "Components/wbcomprosaalarmbox.h"
#include "Components/wbcomprosaalarmtripper.h"
#include "Components/wbcomprosathinkernpctarget.h"
#include "Components/wbcomprosasign.h"
#include "Components/wbcomprosasignreader.h"
#include "Components/wbcomprosaobjectives.h"
#include "Components/wbcomprosareadable.h"
#include "Components/wbcomprosacollectible.h"
#include "Components/wbcomprosasensorrepulsor.h"
#include "Components/wbcomprosastim.h"
#include "Components/wbcomprosajukebox.h"
#include "Components/wbcomprosalinkedentities.h"
#include "Components/wbcomprosacampaignartifact.h"
#include "Components/wbcomprosaheadshot.h"
#include "Components/wbcomprosamapmarker.h"
#include "Components/wbcomprosaspawnmanager.h"
#include "Components/wbcomprosadamageeffects.h"
#include "Components/wbcomprosaloottable.h"
#include "Components/wbcomprosaposeop.h"

#else

ADDWBCOMPONENT( RosaCharacterConfig );	// ROSANOTE: Moved before mesh, transform, and health so they can query it during initialization
ADDWBCOMPONENT( RosaTransform );
ADDWBCOMPONENT( RosaUpgrades );
ADDWBCOMPONENT( RosaPlayer );
ADDWBCOMPONENT( RosaCollision );		// ROSANOTE: Added before doors to fix initialization errors
ADDWBCOMPONENT( RosaMesh );
ADDWBCOMPONENT( RosaCharacter );
ADDWBCOMPONENT( RosaLight );
ADDWBCOMPONENT( RosaAntiLight );
ADDWBCOMPONENT( RosaDecal );
ADDWBCOMPONENT( RosaTrapBolt );
ADDWBCOMPONENT( RosaHealth );
ADDWBCOMPONENT( RosaParticles );
ADDWBCOMPONENT( RosaCamera );
ADDWBCOMPONENT( RosaFrobber );
ADDWBCOMPONENT( RosaFrobbable );		// ROSANOTE: Added before doors to fix initialization errors
ADDWBCOMPONENT( RosaDoor );				// ROSANOTE: Added after collision and frobbables to fix initialization errors
ADDWBCOMPONENT( RosaInventory );
ADDWBCOMPONENT( RosaPickup );
ADDWBCOMPONENT( RosaItem );
ADDWBCOMPONENT( RosaWeapon );
ADDWBCOMPONENT( RosaRope );
ADDWBCOMPONENT( RosaHands );
ADDWBCOMPONENT( RosaVisible );
ADDWBCOMPONENT( RosaSensorVision );
ADDWBCOMPONENT( RosaSensorHearing );
ADDWBCOMPONENT( RosaSensorMarkup );
ADDWBCOMPONENT( RosaSensorDamage );
ADDWBCOMPONENT( RosaSensorTheft );
ADDWBCOMPONENT( RosaThinkerTarget );
ADDWBCOMPONENT( RosaThinkerPatrol );
ADDWBCOMPONENT( RosaAIMotion );
ADDWBCOMPONENT( RosaFaction );
ADDWBCOMPONENT( RosaHeadTracker );
ADDWBCOMPONENT( RosaRagdoll );
ADDWBCOMPONENT( RosaHitbox );
ADDWBCOMPONENT( RosaMarkup );
ADDWBCOMPONENT( RosaSound );
ADDWBCOMPONENT( RosaFootsteps );
ADDWBCOMPONENT( RosaWallet );
ADDWBCOMPONENT( RosaAmmoBag );
ADDWBCOMPONENT( RosaLock );
ADDWBCOMPONENT( RosaRespawner );
ADDWBCOMPONENT( RosaAnchor );
ADDWBCOMPONENT( RosaClimbable );
ADDWBCOMPONENT( RosaHUDMarker );
ADDWBCOMPONENT( RosaPowerTeleport );
ADDWBCOMPONENT( RosaKeyRing );
ADDWBCOMPONENT( RosaMedkit );
ADDWBCOMPONENT( RosaSleeper );
ADDWBCOMPONENT( RosaSpikes );
ADDWBCOMPONENT( RosaIcicles );
ADDWBCOMPONENT( RosaPatrolPath );
ADDWBCOMPONENT( RosaSensorPatrol );
ADDWBCOMPONENT( RosaHackable );
ADDWBCOMPONENT( RosaSensorBump );
ADDWBCOMPONENT( RosaRepulsor );
ADDWBCOMPONENT( RosaSpin );
ADDWBCOMPONENT( RosaSwitch );
ADDWBCOMPONENT( RosaSwitchable );
ADDWBCOMPONENT( RosaSecCam );
ADDWBCOMPONENT( RosaAlarmBox );
ADDWBCOMPONENT( RosaAlarmTripper );
ADDWBCOMPONENT( RosaThinkerNPCTarget );
ADDWBCOMPONENT( RosaSign );
ADDWBCOMPONENT( RosaSignReader );
ADDWBCOMPONENT( RosaObjectives );
ADDWBCOMPONENT( RosaReadable );
ADDWBCOMPONENT( RosaCollectible );
ADDWBCOMPONENT( RosaSensorRepulsor );
ADDWBCOMPONENT( RosaStim );
ADDWBCOMPONENT( RosaJukebox );
ADDWBCOMPONENT( RosaLinkedEntities );
ADDWBCOMPONENT( RosaCampaignArtifact );
ADDWBCOMPONENT( RosaHeadshot );
ADDWBCOMPONENT( RosaMapMarker );
ADDWBCOMPONENT( RosaSpawnManager );
ADDWBCOMPONENT( RosaDamageEffects );
ADDWBCOMPONENT( RosaLootTable );
ADDWBCOMPONENT( RosaPoseOp );

#endif
