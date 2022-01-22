#ifndef ROSAINTRO_H
#define ROSAINTRO_H

#include "vector.h"
#include "angles.h"
#include "interpolator.h"
#include "array.h"
#include "simplestring.h"

class Mesh;

class RosaIntro
{
public:
	RosaIntro();
	~RosaIntro();

	bool	IsRunning() { return m_Phase < EIP_Finished; }

	void	Tick( const float DeltaTime );

private:
	void	Initialize();
	void	ShutDown();

	void	SetView();
	void	RestoreView();

	void	StartPhaseRunning( const float InitialDelay );
	void	StartPhaseFinished();

	enum EIntroPhase
	{
		EIP_None,
		EIP_Running,
		EIP_Finished,
		EIP_COUNT
	};

	// Transient/current vars
	EIntroPhase				m_Phase;
	float					m_CurrentTime;

	float					m_FadeInDuration;

	float					m_FinishedTime;

	float					m_IntroFadeOutTime;
	float					m_IntroFadeOutDuration;

	float					m_TitleFadeInDuration;

	SimpleString			m_ColorGrading;
	SimpleString			m_OldColorGrading;
};

#endif // ROSAINTRO_H
