#ifndef ROSAGAME_H
#define ROSAGAME_H

#include "iwbeventobserver.h"
#include "wbevent.h"
#include "vector2.h"
#include "vector4.h"
#include "matrix.h"

class RosaSaveLoad;
class RosaPersistence;
class RosaCampaign;
class RosaMusic;
class RosaConversation;
class RosaSupertitles;
class RosaHUDLog;
class RosaWardrobe;
class RosaLockpicking;
class Mesh;
class ITexture;
class UIWidgetImage;

typedef WBEvent TPersistence;

#define BUILD_ROSA_TOOLS	BUILD_DEV && BUILD_WINDOWS_NO_SDL

#define ROSA_BLOOM_TAPS		8
#define ROSA_BLOOM_PASSES	3
#define ROSA_BLOOMA_SCALE	2
#define ROSA_BLOOMB_SCALE	4
#define ROSA_BLOOMC_SCALE	8

#define ROSA_USE_FILMIC_POST		1
#define ROSA_USE_WATERCOLOR_POST	(!(ROSA_USE_FILMIC_POST))

// For Eldritch / Dark Souls / Loam style saves, where you get a single save slot per "profile"
// (compared to Neon or Vamp where you can save into any slot). For games with one profile like Zeta,
// this should be disabled; there is no concept of slots in that case and we should use NumAutosaves instead.
#define ROSA_USE_ACTIVESAVESLOT		0

#define ROSA_ALLOW_QUICKSAVELOAD	0 || BUILD_DEV	// i.e., is quicksave/quickload a shipping feature or a dev tool?

#define ROSA_USE_PERSISTENT_WORLDS	0

#define ROSA_USE_MAXIMAP			0 || BUILD_DEV

class RosaGame : public IWBEventObserver
{
public:
	RosaGame();
	~RosaGame();

	void					Initialize();
	void					ShutDown();

	void					Tick( const float DeltaTime );
	void					TickPaused( const float DeltaTime );
	void					Render() const;

	const SimpleString&		GetCurrentLevelName() const { return m_CurrentLevelName; }
	void					SetCurrentLevelName( const SimpleString& LevelName );

	SimpleString			GetCurrentFriendlyLevelName() const;
	static SimpleString		GetFriendlyLevelName( const SimpleString& LevelName );

	void					ClearTravelPersistence();
	TPersistence&			GetTravelPersistence()	{ return m_TravelPersistence; }
	static TPersistence&	StaticGetTravelPersistence();

	RosaSaveLoad*			GetSaveLoad() const		{ return m_SaveLoad; }
	RosaPersistence*		GetPersistence() const	{ return m_GenerationPersistence; }
	RosaCampaign*			GetCampaign() const		{ return m_Campaign; }
	RosaMusic*				GetMusic() const		{ return m_Music; }
	RosaConversation*		GetConversation() const	{ return m_Conversation; }
	RosaSupertitles*		GetSupertitles() const	{ return m_Supertitles; }
	RosaHUDLog*				GetHUDLog() const		{ return m_HUDLog; }
	RosaWardrobe*			GetWardrobe() const		{ return m_Wardrobe; }
	RosaLockpicking*		GetLockpicking() const	{ return m_Lockpicking;}

	void					Autosave() const;

	void					RefreshRTDependentSystems();
	Mesh*					CreateFullscreenQuad( const uint Width, const uint Height, const HashedString& PrescribedBucket, const SimpleString& MaterialDef );
	void					CreateGlobalAmbientQuad();
	void					CreateUpscaleQuad();
	void					CreatePostQuad();
	void					CreateSSAOQuad();
	void					CreateGradientQuad();
	void					CreateEdgeQuad();
	void					CreateLightCombineQuad();
#if ROSA_USE_WATERCOLOR_POST
	void					CreateEdgeQuad();
#endif
	void					CreateBloomQuads();
	void					CreateFXAAQuad();
	void					CreateMinimapBQuad();
	void					CreateMinimapFXAAQuad();
	void					UpdateMinimap();
#if ROSA_USE_MAXIMAP
	void					CreateMaximapBQuad();
	void					CreateMaximapFXAAQuad();
	void					UpdateMaximap();
#endif
	void					SetGlobalCubemap( ITexture* const pCubemap );
	ITexture*				GetGlobalCubemap() const { return m_GlobalCubemap; }
	Mesh*					GetGlobalAmbientQuad() const { return m_GlobalAmbientQuad; }
	Mesh*					GetUpscaleQuad() const { return m_UpscaleQuad; }
	Mesh*					GetPostQuad() const { return m_PostQuad; }
	Mesh*					GetPostCheapQuad() const { return m_PostCheapQuad; }
	Mesh*					GetSSAOQuad() const { return m_SSAOQuad; }
	Mesh*					GetGradientQuad() const { return m_GradientQuad; }
	Mesh*					GetEdgeQuad() const { return m_EdgeQuad; }
	Mesh*					GetLightCombineQuad() const { return m_LightCombineQuad; }
#if ROSA_USE_WATERCOLOR_POST
	Mesh*					GetEdgeQuad() const { return m_EdgeQuad; }
#endif
	Mesh*					GetBloomQuad( const uint Index ) const { return m_BloomQuads[ Index ]; }
	Mesh*					GetFXAAQuad() const { return m_FXAAQuad; }
	Mesh*					GetMinimapBQuad() const { return m_MinimapBQuad; }
	Mesh*					GetMinimapFXAAQuad() const { return m_MinimapFXAAQuad; }
#if ROSA_USE_MAXIMAP
	Mesh*					GetMaximapBQuad() const { return m_MaximapBQuad; }
	Mesh*					GetMaximapFXAAQuad() const { return m_MaximapFXAAQuad; }
#endif
	void					SetMinimapTextures( const SimpleString& TonesFilename, const SimpleString& FloorFilename, const SimpleString& SolidFilename );
	float					GetMinimapHeightThreshold() const { return m_MinimapHeightThreshold; }
	float					GetMinimapHeightOffset() const { return m_MinimapHeightOffset; }
	float					GetMinimapHeightDiffScale() const { return m_MinimapHeightDiffScale; }
	float					GetMinimapHeightToneScale() const { return m_MinimapHeightToneScale; }
	float					GetMinimapRenderEdges() const { return m_MinimapRenderEdges; }
	float					GetMinimapRcpTileSize() const { return m_MinimapRcpTileSize; }
	const SimpleString&		GetColorGradingTexture() const { return m_ColorGradingTexture; }
	void					SetColorGradingTexture( const SimpleString& TextureFilename );
	void					UpdateColorGradingEnabled();
	void					SetBloomKernelTexture( const SimpleString& TextureFilename );

	void					SetNoiseTexture( const SimpleString& TextureFilename );
	void					UpdateNoiseEnabled();
	void					SetNoiseScaleRange( const float NoiseScaleLo, const float NoiseScaleHi );
	Vector2					GetNoiseScaleRange() const { return m_NoiseScaleRange; }
	void					SetNoiseRange( const float NoiseRange );
	float					GetNoiseRange() const { return m_NoiseRange; }

	void					UpdateGraphicsOptionWidgets();

	void					UpdateBloomEnabled();
	void					SetDirtyLensTexture( const SimpleString& TextureFilename );
	void					UpdateDirtyLensEnabled();
	void					UpdateHalosEnabled();
	bool					GetHalosEnabled() const { return m_HalosEnabled; }

	void					UpdateBlurEnabled();
	void					UpdateEdgeEnabled();

	void					SetDisplaceTexture( const SimpleString& TextureFilename );
	void					UpdateDisplaceEnabled();

	void					SetBlotTexture( const SimpleString& TextureFilename );
	void					UpdateBlotEnabled();

	void					SetCanvasTexture( const SimpleString& TextureFilename );
	void					UpdateCanvasEnabled();

	void					UpdatePostCheapEnabled();

	void					SetWatercolorParams( const Vector4& WatercolorParams, const float DisplacePct ) { m_WatercolorParams = WatercolorParams; m_DisplacePct = DisplacePct; }
	const Vector4&			GetWatercolorParams() const { return m_WatercolorParams; }
	const float				GetDisplacePct() const { return m_DisplacePct; }
	//void					SetEdgeColors( const Vector4& EdgeBackColor, const Vector4& EdgeColor ) { m_EdgeBackColor = EdgeBackColor; m_EdgeColor = EdgeColor; }
	void					SetEdgeColorHSV( const Vector& EdgeColorHSV );
	const Vector4&			GetEdgeBackColor() const { return m_EdgeBackColor; }
	const Vector4&			GetEdgeColor() const { return m_EdgeColor; }
	void					SetEdgeLuminanceParams( const float EdgeLuminanceLo, const float EdgeLuminanceHi ) { m_EdgeLuminanceMul = 1.0f / ( EdgeLuminanceHi - EdgeLuminanceLo ); m_EdgeLuminanceAdd = -EdgeLuminanceLo * m_EdgeLuminanceMul; }
	float					GetEdgeLuminanceMul() const { return m_EdgeLuminanceMul; }
	float					GetEdgeLuminanceAdd() const { return m_EdgeLuminanceAdd; }

	void					SetGamma( const float Gamma ) { m_Gamma = Gamma; }
	float					GetGamma() const { return m_Gamma; }

	void					SetBloomRadius( const float VerticalRadius, const float AspectRatio );	// VerticalRadius as fraction of screen size
	void					RefreshBloomParams();
	Vector2					GetBloomRadius();
	void					SetBloomParams( const Vector& Threshold, const float Scalar ) { m_BloomParams = Vector4( Threshold, Scalar ); }
	const Vector4&			GetBloomParams() { return m_BloomParams; }

	void					SetFogColors( const Vector4& FogColorNearLo, const Vector4& FogColorFarLo, const Vector4& FogColorNearHi, const Vector4& FogColorFarHi );
	void					SetFogCurves( const Vector4& FogNearFarCurve, const Vector4& FogLoHiCurve );
	void					SetFogParams( const float FogNear, const float FogFar, const float EmissiveMax, const float Exposure, const float FogLightDensity );
	void					UpdateFogEnabled();
	void					SetHeightFogParams( const float HeightFogLo, const float HeightFogHi, const float HeightFogExp, const float HeightFogLoExp, const float HeightFogHiExp );
	void					SetExposure( const float Exposure ) { m_FogParams.w = Exposure; m_FogLightParams.y = Exposure; m_InvExposure = 1.0f / Exposure; }
	void					AdjustExposure( const float Exposure, const float DeltaTime );
	float					GetInvExposure() const { return m_InvExposure; }
	const bool				GetFogEnabled() const { return m_FogEnabled; }
	const Matrix&			GetFogColors() const { return m_FogColors; }
	const Vector4&			GetFogNearFarCurve() const { return m_FogNearFarCurve; }
	const Vector4&			GetFogLoHiCurve() const { return m_FogLoHiCurve; }
	const Vector4&			GetFogParams() const { return m_FogParams; }
	const Vector4&			GetHeightFogParams() const { return m_HeightFogParams; }
	const Vector4&			GetRegionFogScalar() const { return m_RegionFogScalar; }
	const Vector4&			GetFogLightParams() const { return m_FogLightParams; }
	void					SetRegionFogScalar( const Vector4& RegionFogScalar ) { m_RegionFogScalar = RegionFogScalar; }
	void					AdjustFogRegionScalar( const Vector4& RegionFogScalar, const float DeltaTime );

	void					AdjustEdgeColorHSV( const Vector& EdgeColorHSV, const float DeltaTime );

	void					SetSkyParams( const Vector4& SunVector, const Vector4& SkyColorHi, const Vector4& SkyColorLo );
	const Vector4&			GetSunVector() const { return m_SunVector; }
	const Vector4&			GetSkyColorHi() const { return m_SkyColorHi; }
	const Vector4&			GetSkyColorLo() const { return m_SkyColorLo; }

	void					SetWindMatrix( const Matrix& WindMatrix )	{ m_WindMatrix = WindMatrix; }
	const Matrix&			GetWindMatrix() const						{ return m_WindMatrix; }
	void					SetWindPhaseVectors( const Vector4& WindPhaseTime, const Vector4& WindPhaseSpace )	{ m_WindPhaseTime = WindPhaseTime; m_WindPhaseSpace = WindPhaseSpace; }
	const Vector4&			GetWindPhaseTime() const					{ return m_WindPhaseTime; }
	const Vector4&			GetWindPhaseSpace() const					{ return m_WindPhaseSpace; }

	void					SetWindWaterVector( const Vector4& WindWaterVector )	{ m_WindWaterVector = WindWaterVector; }
	const Vector4&			GetWindWaterVector() const								{ return m_WindWaterVector; }

	void					AdjustMinimapScalar( const float MinimapScalar, const float DeltaTime );
	void					SetMinimapScalar( const float MinimapScalar )	{ m_CurrentMinimapScalar = MinimapScalar; }
	float					GetMinimapScalar() const						{ return m_CurrentMinimapScalar; }

	void					SetMusic( const SimpleString& Music )		{ m_CurrentMusic	= Music; }
	void					SetAmbience( const SimpleString& Ambience );
	void					SetReverb( const SimpleString& ReverbDef );

	void					RequestReturnToTitle();
	void					RequestLoadSlot( const SimpleString& Slot );

	// IWBEventObserver
	virtual void			HandleEvent( const WBEvent& Event );

	SimpleString			GetTitleScreenLevelName() const;
	SimpleString			GetInitialLevelName() const;
	SimpleString			GetHubLevelName() const;
	bool					IsInTitleScreen() const;
	bool					IsInHub() const;			// ROSANOTE: This now queries a bool in the level def instead of comparing the name
	bool					ShouldLowerWeapon() const;	// ROSANOTE: Generally the same as IsInHub but excludes Intro level
	void					SetUIRetreatDisabled( const bool Disabled );
	void					RefreshUIRetreatEnabled();

	void					LaunchWebSite( const SimpleString& URL );
	void					OpenUserDataPath();

	// Helper function, because where else would it go
	static WBEntity*		GetPlayer();
	static Vector			GetPlayerLocation();
	static Angles			GetPlayerOrientation();
	static Vector			GetPlayerViewLocation();
	static Angles			GetPlayerViewOrientation();
	static Vector			GetPlayerFeetLocation();
	// HACKHACK so I'm not doing so many lookups for player feet in minimap SDPs
	static void				CachePlayerFeetLocation();
	static Vector&			GetCachedPlayerFeetLocation();
	static bool				IsPlayerAlive();
	static bool				IsPlayerDisablingPause();
	static bool				IsPlayerVisible();
	static bool				IsGamePaused();

	// More helpers
	static UIWidgetImage*	GetMinimapImage();
#if ROSA_USE_MAXIMAP
	static UIWidgetImage*	GetMaximapImage();
#endif

private:
	void				RequestGoToInitialLevel();
	void				RequestGoToHubLevel();
	void				RequestGoToNextLevel( const HashedString& TeleportLabel );
	void				RequestGoToPrevLevel( const HashedString& TeleportLabel );
	void				RequestGoToLevel( const SimpleString& NextLevel, const HashedString& TeleportLabel, const bool SuppressLoadingScreen );
	void				RequestGoToLevelInternal( const SimpleString& NextLevel, const HashedString& TeleportLabel, const bool RestoreSpawnPoint, const bool SuppressLoadingScreen );
	void				GoToLevel();
	void				PreGoToLevel();
	void				PostGoToLevel();

	void				LoadSlot();

	SimpleString		DecorateWorldFileName( const SimpleString& LevelName ) const;

	uint				m_GoToLevelInNumTicks;
	bool				m_IsRestarting;			// Completely restarting game, clearing persistence (i.e., returning to title or starting a new game)
	bool				m_IsReturningToHub;		// Returning to hub within a game, flushing world files but keeping persistence
	bool				m_RestoreSpawnPoint;
	HashedString		m_TeleportLabel;
	SimpleString		m_NextLevelName;

	uint				m_LoadSlotInNumTicks;
	SimpleString		m_LoadSlotName;

	SimpleString		m_CurrentLevelName;

	TPersistence		m_TravelPersistence;		// Travel persistence propagates world state data between worlds

	RosaSaveLoad*		m_SaveLoad;
	RosaPersistence*	m_GenerationPersistence;	// Generation persistence saves progress beyond death (this is only tutorials and achievements currently)
	RosaCampaign*		m_Campaign;
	RosaMusic*			m_Music;
	RosaConversation*	m_Conversation;
	RosaSupertitles*	m_Supertitles;
	RosaHUDLog*			m_HUDLog;
	RosaWardrobe*		m_Wardrobe;
	RosaLockpicking*	m_Lockpicking;

	float				m_Gamma;
	ITexture*			m_GlobalCubemap;
	Mesh*				m_GlobalAmbientQuad;
	Mesh*				m_UpscaleQuad;
	Mesh*				m_PostQuad;
	Mesh*				m_PostCheapQuad;
	bool				m_UsePostCheapQuad;
#if BUILD_ROSA_TOOLS
	Mesh*				m_PostToolsQuad;
#endif
	Mesh*				m_SSAOQuad;
	Mesh*				m_GradientQuad;
	Mesh*				m_EdgeQuad;
	Mesh*				m_LightCombineQuad;	// ROSANOTE: For deferred light combine
#if ROSA_USE_WATERCOLOR_POST
	Mesh*				m_EdgeQuad;
#endif
	Array<Mesh*>		m_BloomQuads;
	Mesh*				m_FXAAQuad;
	Mesh*				m_MinimapBQuad;		// For doing the edge detection pass
	Mesh*				m_MinimapFXAAQuad;
	SimpleString		m_MinimapTonesTexture;
	SimpleString		m_MinimapFloorTexture;
	SimpleString		m_MinimapSolidTexture;
	float				m_MinimapHeightThreshold;
	float				m_MinimapHeightOffset;
	float				m_MinimapHeightDiffScale;
	float				m_MinimapHeightToneScale;
	float				m_MinimapRenderEdges;
	float				m_MinimapRcpTileSize;
#if ROSA_USE_MAXIMAP
	Mesh*				m_MaximapBQuad;		// For doing the edge detection pass
	Mesh*				m_MaximapFXAAQuad;
#endif
	SimpleString		m_ColorGradingTexture;
	SimpleString		m_NoiseTexture;
	Vector2				m_NoiseScaleRange;
	float				m_NoiseRange;
	bool				m_HalosEnabled;
	SimpleString		m_DirtyLensTexture;
	SimpleString		m_BloomKernelTexture;
	SimpleString		m_DisplaceTexture;		// Watercolor
	SimpleString		m_BlotTexture;			// Watercolor
	SimpleString		m_CanvasTexture;		// Watercolor
	Vector				m_EdgeColorHSV;
	Vector4				m_EdgeBackColor;
	Vector4				m_EdgeColor;
	float				m_EdgeLuminanceMul;
	float				m_EdgeLuminanceAdd;
	Vector4				m_WatercolorParams;
	float				m_DisplacePct;
	bool				m_FogEnabled;
	Matrix				m_FogColors;
	Vector4				m_FogNearFarCurve;
	Vector4				m_FogLoHiCurve;
	Vector4				m_FogParams;		// x = near, y = 1/(far-near), z = emissive scalar (HACKHACK), w = exposure scalar (HACKHACK)
	Vector4				m_HeightFogParams;	// x = lo, y = 1/range, z = lo exp, w = hi exp
	Vector4				m_RegionFogScalar;	// RGBA linear space multiplier on global fog
	Vector4				m_FogLightParams;	// x = density, y = exposure scalar (HACKHACK)
	Vector4				m_SunVector;
	Vector4				m_SkyColorHi;
	Vector4				m_SkyColorLo;
	Matrix				m_WindMatrix;
	Vector4				m_WindPhaseTime;
	Vector4				m_WindPhaseSpace;
	Vector4				m_WindWaterVector;
	float				m_InvExposure;
	float				m_BloomVerticalRadius;
	float				m_BloomAspectRatio;
	Vector2				m_BloomStepRadiusH;
	Vector2				m_BloomStepRadiusV;
	Vector4				m_BloomParams;		// xyz = RGB threshold, w = scalar
	bool				m_ReturnVerticalBloomRadius;
	float				m_CurrentMinimapScalar;
	SimpleString		m_CurrentMusic;
	SimpleString		m_CurrentAmbience;
	SimpleString		m_CurrentReverbDef;
};

#endif // ROSAGAME_H
