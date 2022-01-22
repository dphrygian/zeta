#ifndef IBONEMODIFIER_H
#define IBONEMODIFIER_H

struct	SBoneInfo;
struct	SBone;
class	Matrix;

class IBoneModifier
{
public:
	virtual ~IBoneModifier() {}

	virtual void	ModifyBone( const SBoneInfo& BoneInfo, const uint BoneIndex, const Matrix& ParentBoneMatrix, SBone& InOutBone ) = 0;
};

#endif // IBONEMODIFIER_H
