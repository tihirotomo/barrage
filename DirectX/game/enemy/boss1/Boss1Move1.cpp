#include "Boss1Move1.h"
#include "../boss1/Boss1Model.h"
#include "../BaseBoss.h"
#include "Math/Easing/Easing.h"
#include "GameHelper.h"

Boss1Move1::Boss1Move1()
{
	boss->GetBaseModel()->AnimationReset();
	boss->GetBaseModel()->SetAnimation(int(Boss1Model::Movement::walk));
	DirectX::XMFLOAT3 rota = {};
	SetMoveRotation(rota);
	boss->GetCenter()->SetRotation(rota);
	moveTime = std::make_unique<Engine::Timer>();

	hitTimer = std::make_unique<Engine::Timer>();
}

void Boss1Move1::Update()
{
	const float maxTimer = 100.0f;

	boss->SetMoveVec(boss->GetBaseModel()->GetModelMove());

	moveTime->Update();

	if (*moveTime.get() <= maxTimer) { return; }
	isEnd = true;
}

void Boss1Move1::SetMoveRotation(DirectX::XMFLOAT3& _rota)
{
	//どの方向に移動するか
	//後から条件書く
	_rota.y = RandomFloat(360.0f);
}
