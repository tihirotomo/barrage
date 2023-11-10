#pragma once
#include "Object/3d/Fbx.h"

class BaseBossModel
{
public:
	virtual ~BaseBossModel() {};

	virtual void Update() {};

	virtual void Draw() {};

	DirectX::XMFLOAT3 GetModelMove() { return fbxObject->GetModelMove(); }
	void AnimationReset() { fbxObject->AnimationReset(); }
	void SetAnimation(const int _animationNumber) { fbxObject->SetUseAnimation(_animationNumber); }
	Base3D* GetObjectInst() { return fbxObject.get(); }
	void RrameReset() { fbxObject->FrameReset(); }

protected:
	
	std::unique_ptr<FbxModel> model;
	std::unique_ptr<Fbx> fbxObject;
};

