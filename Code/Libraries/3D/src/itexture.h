#ifndef ITEXTURE_H
#define ITEXTURE_H

struct SSamplerState;

class ITexture
{
public:
	virtual ~ITexture() {}

	virtual void*			GetHandle() = 0;
	virtual SSamplerState*	GetSamplerState() = 0;
	virtual bool			IsCubemap() const = 0;
};

#endif // ITEXTURE_H
