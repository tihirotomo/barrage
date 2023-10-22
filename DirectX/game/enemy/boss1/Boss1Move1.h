#pragma once
#include "../game/enemy/BaseAction.h"
#include <DirectXMath.h>
#include "../Math/Timer.h"

/// <summary>
/// 直線移動（イージングあり）
/// </summary>
class Boss1Move1 : public BaseAction
{
public:
	Boss1Move1(const DirectX::XMFLOAT3& _pos = {});
	~Boss1Move1() {};

	void Update() override;

	void Draw() override {};

	void FrameReset() override {};

	void GetAttackCollision(std::vector<BaseAction::AttackCollision>& _info) override {};

private:

	//開始地点
	DirectX::XMFLOAT3 startPos;
	//移動後地点
	DirectX::XMFLOAT3 endPos;
	//イージングタイマー
	std::unique_ptr<Engine::Timer> timer;
};

