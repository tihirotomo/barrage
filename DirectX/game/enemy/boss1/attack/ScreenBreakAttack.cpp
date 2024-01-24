#include "ScreenBreakAttack.h"
#include "GameHelper.h"
#include "../Boss1Model.h"
#include "../game/enemy/BaseBoss.h"
#include "../Math/Easing/Easing.h"
#include "../game/effect/ScreenCut.h"

ScreenBreakAttack::ScreenBreakAttack()
{
	boss->SetPlayerDirection();
	boss->GetBaseModel()->SetAnimation(int(Boss1Model::Movement::screen_break_attack));
	boss->GetBaseModel()->AnimationReset();
	boss->GetBaseModel()->SetAnimation(int(Boss1Model::Movement::standBy));

	useCollision = UseCollision::capsule;
	state = State::fieldMove;

	timer = std::make_unique<Engine::Timer>();
	isEnd = false;

	hitTimer = std::make_unique<Engine::Timer>();

	isCollision = false;

	boss->GetCenter()->SetOutline(false);

	boss->SetIsCollider(false);

	panelBreak = false;

	moveBefore = boss->GetBaseModel()->GetPosition();

	func_.emplace_back([this] {return FieldMove(); });
	func_.emplace_back([this] {return Move1Start(); });
	func_.emplace_back([this] {return Move1End(); });
	func_.emplace_back([this] {return AttackWait(); });
	func_.emplace_back([this] {return Attack(); });
	func_.emplace_back([this] {return Move2Start(); });
	func_.emplace_back([this] {return Move2End(); });
}

void ScreenBreakAttack::Update()
{
	//動き
	if (state != State::non) {
		func_[int(state)]();
	}

	timer->Update();
}

void ScreenBreakAttack::GetAttackCollisionCapsule(std::vector<Capsule>& _info)
{
	Capsule add;
	add.startPosition = boss->GetBaseModel()->GetPartsPos("larm1");
	add.endPosition = boss->GetBaseModel()->GetPartsPos("larm2");
	add.radius = 100.0f;
	_info.emplace_back(add);
}

void ScreenBreakAttack::FieldMove()
{
	const float maxTime = 50.0f;
	const float rate = *timer.get() / maxTime;

	Vector3 pos = {};
	pos.x = Easing::OutCubic(moveBefore.x, GameHelper::Instance()->GetStageSize() / 2.0f, rate);
	pos.y = Easing::OutCubic(moveBefore.y, 10.0f, rate);
	pos.z = Easing::OutCubic(moveBefore.z, GameHelper::Instance()->GetStageSize(), rate);

	boss->GetBaseModel()->SetPosition(pos);

	if (*timer.get() < maxTime) { return; }
	state = State::move1Start;
	timer->Reset();
}

void ScreenBreakAttack::Move1Start()
{
	const float maxTime = 50.0f;
	boss->GetCenter()->SetColor({ 1.0f,1.0f,1.0f,Easing::Lerp(1.0f, 0.0f, *timer.get() / maxTime) });

	if (*timer.get() < maxTime) { return; }
	boss->GetBaseModel()->SetPosition({ GameHelper::Instance()->GetStageSize() / 2.0f ,
		-460.0f,GameHelper::Instance()->GetStageSize() + 40.0f });

	//サイズ変更
	const float scale = 0.04f;
	boss->GetBaseModel()->GetObjectInst()->SetScale({scale, scale, scale});

	timer->Reset();
	state = State::move1End;
}

void ScreenBreakAttack::Move1End()
{
	const float maxTime = 50.0f;
	boss->GetCenter()->SetColor({ 1.0f,1.0f,1.0f,Easing::Lerp(0.0f, 1.0f, *timer.get() / maxTime) });

	if (*timer.get() < maxTime) { return; }
	timer->Reset();
	state = State::attackWait;
}

void ScreenBreakAttack::AttackWait()
{
	if (*timer.get() < 200.0f) { return; }
	timer->Reset();
	isCollision = true;
	ScreenCut::Reset();
	state = State::attack;
	boss->GetBaseModel()->SetAnimation(int(Boss1Model::Movement::screen_break_attack));
}

void ScreenBreakAttack::Attack()
{
	if (*timer.get() > 90.0f && !panelBreak && !isCollision) {
		ScreenCut::SetEffect(true);
		panelBreak = true;
	}

	if (!boss->GetBaseModel()->GetIsAnimationEnd()) { return; }
	timer->Reset();
	state = State::move2Start;
	boss->GetBaseModel()->SetAnimation(int(Boss1Model::Movement::standBy));
	isCollision = false;
}

void ScreenBreakAttack::Move2Start()
{
	const float maxTime = 50.0f;
	boss->GetCenter()->SetColor({ 1.0f,1.0f,1.0f,Easing::Lerp(1.0f, 0.0f, *timer.get() / maxTime) });
	
	if (*timer.get() < maxTime) { return; }
	boss->GetBaseModel()->SetPosition({ GameHelper::Instance()->GetStageSize() / 2.0f ,
		10.0f,GameHelper::Instance()->GetStageSize() / 2.0f });
	boss->GetBaseModel()->GetObjectInst()->SetScale({ 0.0011f,0.0011f ,0.0011f });

	timer->Reset();
	state = State::move2End;
}

void ScreenBreakAttack::Move2End()
{
	const float maxTime = 50.0f;
	boss->GetCenter()->SetColor({ 1.0f,1.0f,1.0f,Easing::Lerp(0.0f, 1.0f, *timer.get() / maxTime) });

	if (*timer.get() < maxTime) { return; }
	timer->Reset();
	boss->SetIsCollider(true);
	boss->GetCenter()->SetOutline(true);
	state = State::non;
	isEnd = true;
}