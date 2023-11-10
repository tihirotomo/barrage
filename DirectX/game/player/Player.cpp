﻿#include "Player.h"
#include "system/GameInputManager.h"
#include "GameHelper.h"
#include "Object/3d/collider/SphereCollider.h"
#include "Object/3d/collider/CollisionManager.h"
#include "Object/3d/collider/CollisionAttribute.h"
#include "Math/Vector2.h"
#include "game/camera/GameCamera.h"
#include "engine/Math/Easing/Easing.h"
#include "PlayerSwordAttack1.h"
#include <imgui.h>

using namespace DirectX;

const XMFLOAT3 Player::moveMinPos = { 0,0,0 };
const XMFLOAT3 Player::moveMaxPos = { 510,0,510 };
float Player::jumpPower = 3.0f;
float Player::gravityAccel = -0.015f;
const float Player::moveSpeedMax = 1.25f;
const float Player::dashSpeedMax = 2.5f;

Player::Player()
{
	model = FbxModel::Create("player");
	object = Fbx::Create(model.get());
	object->SetShadowMap(true);
	object->SetAnimation(true);

	//剣モデル読み込み
	swordModel = Model::CreateFromOBJ("sword");
	std::string bone = "mixamorig:RightHand";
	XMMATRIX matScale = XMMatrixScaling(100.0f, 100.0f, 100.0f);
	XMMATRIX matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(XMConvertToRadians(0.0f));
	matRot *= XMMatrixRotationX(XMConvertToRadians(0.0f));
	matRot *= XMMatrixRotationY(XMConvertToRadians(0.0f));
	DirectX::XMMATRIX matTrans = XMMatrixTranslation(0.0f, 0.0f, 0.0f);
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();
	world *= matScale;
	world *= matRot;
	world *= matTrans;
	object->SetBoneObject(bone, "rightHand", swordModel.get(), world);

	pos = { 100.0f,200.0f,100.0f };
	//連続ジャンプ可能回数設定
	jumpMaxNum = 2;

	//最大体力をセット
	maxHP = 100;
	HP = maxHP;
	hpGauge = std::make_unique<Gauge>(DirectX::XMFLOAT2({ 20.0f, 50.0f }), 600.0f, maxHP, HP, DirectX::XMFLOAT4({ 0.6f, 0.1f, 0.1f, 1.0f }));

	//最大持久力をセット
	maxEndurance = 200;
	endurance = maxEndurance;
	enduranceGauge = std::make_unique<Gauge>(DirectX::XMFLOAT2({ 20.0f, 90.0f }), 600.0f, maxEndurance, endurance, DirectX::XMFLOAT4({ 0.1f, 0.6f, 0.1f, 1.0f }));

	//タイマークラス
	avoidTimer = std::make_unique<Engine::Timer>();
	blinkTimer = std::make_unique<Engine::Timer>();
	healTimer = std::make_unique<Engine::Timer>();
	enduranceRecoveryStartTimer = std::make_unique<Engine::Timer>();
}

void Player::Update()
{
	//毎フレーム戻しておく
	isMoveKey = false;
	isMovePad = false;

	if (isKnockback) {
		Knockback();
	}
	else if (isBlink) {
		Blink();
	}
	else if (isAvoid) {
		Avoid();
	}
	else {
		Move();
		Jump();
		Attack();
		AvoidStart();
		BlinkStart();
	}

	if (!isBlink) {
		Fall();
	}

	HealHPMove();
	EnduranceRecovery();

	//更新した座標などを反映し、オブジェクト更新
	ObjectUpdate();

	hpGauge->Update();
	enduranceGauge->Update();

	if (attackAction) {
		attackAction->Update();
		//攻撃の行動が終了したら解放
		if (attackAction->GetIsAttackActionEnd()) {
			attackAction.release();
			isAttack = false;
		}
	}
}

void Player::Draw()
{
	object->BoneDraw();
	object->Draw();

	hpGauge->Draw();
	enduranceGauge->Draw();

	if (attackAction) {
		attackAction->Draw();
	}
}

void Player::FrameReset()
{
	object->FrameReset();
}

void Player::ImguiDraw()
{
	ImGui::SliderFloat("playerFallAccel", &gravityAccel, 0.0f, -0.5f);
	ImGui::SliderFloat("playerJumpPower", &jumpPower, 0.0f, 20.0f);
}

void Player::DrawLightView()
{
	object->DrawLightView();

	if (attackAction) {
		attackAction->DrawLightView();
	}
}

void Player::Damage(int damageNum, const Vector3& subjectPos)
{
	//HPからダメージ量を引く
	HP -= damageNum;
	HP = max(HP, 0);

	hpGauge->ChangeLength(HP, true);

	//回復中なら回復を中断
	isHeal = false;
	//ブリンク中なら中断
	isBlink = false;

	//ノックバック状態にする
	KnockbackStart(subjectPos, damageNum);

	//HPが0以下なら死亡
	if (!(HP <= 0)) { return; }

	isDead = true;
}

void Player::Heal(int healNum)
{
	//回復前と回復後のHP量を計算
	healBeforeHP = HP;
	healAfterHP = HP + healNum;
	healAfterHP = min(healAfterHP, maxHP);

	//回復状態にする
	isHeal = true;
	healTimer = 0;
}

void Player::PushBack(const XMVECTOR& reject)
{
	//押し戻し
	Vector3 rejectNum = { reject.m128_f32[0],reject.m128_f32[1], reject.m128_f32[2] };
	pos += rejectNum;
}

void Player::ObjectUpdate()
{
	//速度を加算して座標更新
	pos += velocity;

	//壁判定
	pos.x = max(pos.x, moveMinPos.x);
	pos.x = min(pos.x, moveMaxPos.x);
	pos.z = max(pos.z, moveMinPos.z);
	pos.z = min(pos.z, moveMaxPos.z);

	//地面に接地判定
	const float modelHeight = 5; //スケール1のときのモデルの高さ
	if (pos.y <= object->GetScale().y * 2 * modelHeight) {
		pos.y = object->GetScale().y * 2 * modelHeight;
		if (!onGround) {
			onGround = true;
			fallSpeed = 0;
			jumpCount = 0;
			isBlinkStart = true; //ブリンク開始可能にする
		}
	}
	//最終的な座標をセット
	object->SetPosition(pos);

	//オブジェクト更新
	object->Update();
}

void Player::Move()
{
	DirectInput* input = DirectInput::GetInstance();

	//移動キー入力を判定
	isMoveKey = (input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveRight).key) ||
		input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveLeft).key) ||
		input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveForward).key) ||
		input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveBack).key));

	//ある程度スティックを傾けないと移動パッド入力判定しない
	const XMFLOAT2 padIncline = GameInputManager::GetPadLStickIncline();
	isMovePad = (fabsf(padIncline.x) >= GameInputManager::GetPadStickInputIncline() || fabsf(padIncline.y) >= GameInputManager::GetPadStickInputIncline());

	//ダッシュ
	Dash();

	//入力
	const float moveAccel = 0.1f;
	if (isMoveKey || isMovePad) {
		moveSpeed += moveAccel;
		if (isDash) { moveSpeed = min(moveSpeed, dashSpeedMax); }
		else { moveSpeed = min(moveSpeed, moveSpeedMax); }

		Vector3 inputMoveVec{};

		if (isMoveKey) {
			if (input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveRight).key)) {
				inputMoveVec.x = 1;
				inputMoveVec.z = 0;
			}
			if (input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveLeft).key)) {
				inputMoveVec.x = -1;
				inputMoveVec.z = 0;
			}
			if (input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveForward).key)) {
				inputMoveVec.x = 0;
				inputMoveVec.z = 1;
			}
			if (input->PushKey(GameInputManager::GetKeyInputActionData(GameInputManager::MoveBack).key)) {
				inputMoveVec.x = 0;
				inputMoveVec.z = -1;
			}
		}
		if (isMovePad) {
			//パッドスティックの方向をベクトル化
			inputMoveVec.x = GameInputManager::GetPadLStickIncline().x;
			inputMoveVec.z = GameInputManager::GetPadLStickIncline().y;
		}

		//ベクトルをカメラの傾きで回転させる
		inputMoveVec.normalize();
		const float cameraRotaRadian = XMConvertToRadians(-gameCamera->GetCameraRota().y);
		moveVec.x = inputMoveVec.x * cosf(cameraRotaRadian) - inputMoveVec.z * sinf(cameraRotaRadian);
		moveVec.z = inputMoveVec.x * sinf(cameraRotaRadian) + inputMoveVec.z * cosf(cameraRotaRadian);

		//進行方向を向くようにする
		Vector3 moveRotaVelocity = { moveVec.x, 0, moveVec.z };//プレイヤー回転にジャンプは関係ないので、速度Yは0にしておく
		rota = VelocityRotate(moveRotaVelocity);
		object->SetRotation(rota);
	}
	else {
		moveSpeed -= moveAccel;
		moveSpeed = max(moveSpeed, 0);
	}

	//速度をセット
	velocity.x = moveVec.x * moveSpeed;
	velocity.z = moveVec.z * moveSpeed;
}

void Player::Dash()
{
	//地面にいない場合は、変更を受け付けないで抜ける
	if (!onGround) { return; }

	if (!isDash) {
		//ダッシュ開始可能で地面接地しているかつ、ダッシュ入力があって移動した場合にダッシュ状態にする
		if (isDashStart && (isMoveKey || isMovePad) && endurance > 0 && GameInputManager::PushInputAction(GameInputManager::Avoid_Blink_Dash)) {
			isDash = true;
			isDashStart = false;
		}

		//ダッシュ開始不能時は、ダッシュボタン入力を一度離すことで可能になる
		if ((!isDashStart) && (!GameInputManager::PushInputAction(GameInputManager::Avoid_Blink_Dash))) {
			isDashStart = true;
		}
	}
	else {
		//移動 & 入力中はダッシュ状態を維持
		if ((isMoveKey || isMovePad) && endurance > 0 && GameInputManager::PushInputAction(GameInputManager::Avoid_Blink_Dash)) {
			UseEndurance(dashUseEndurance, 1, false); //持久力を使用
		}
		//入力が途切れたときにダッシュを終了する
		else {
			isDash = false;
		}
	}
}

void Player::Fall()
{
	//地面に接地していたら抜ける
	if (onGround) { return; }

	float fallAcc = gravityAccel;

	//ジャンプ中で入力をし続けている場合は落下速度を減少させる
	if (jumpCount >= 1 && isInputJump && GameInputManager::PushInputAction(GameInputManager::Jump)) {
		fallAcc /= 3.5f;
	}
	else {
		isInputJump = false;
	}

	// 加速
	fallSpeed += fallAcc;
	velocity.y += fallSpeed;

	const float fallSppedMax = -10.0f;
	velocity.y = max(velocity.y, fallSppedMax);
}

void Player::AvoidStart()
{
	//地面にいない場合は抜ける
	if (!onGround) { return; }
	//移動中に回避入力がなければ抜ける
	if (!((isMoveKey || isMovePad) && GameInputManager::TriggerInputAction(GameInputManager::Avoid_Blink_Dash))) { return; }
	//持久力が回避で使用する値以下なら抜ける	
	if (endurance < avoidUseEndurance) { return; }
	UseEndurance(avoidUseEndurance, 30, true); //持久力を使用

	//回避するベクトルを求める(現在向いている方向)
	const float rotaRadian = XMConvertToRadians(rota.y - 90);
	avoidVec.x = cosf(rotaRadian);
	avoidVec.z = -sinf(rotaRadian);

	avoidTimer->Reset();
	isAvoid = true;
	isDash = false;
	isDashStart = true;
}

void Player::Avoid()
{
	//タイマー更新
	const float avoidTime = 30;
	avoidTimer->Update();
	const float time = *avoidTimer.get() / avoidTime;

	const float power = Easing::OutCirc(10, 1, time);

	velocity = avoidVec.normalize() * power;

	rota.x = Easing::OutCubic(0, 360, time);
	object->SetRotation(rota);

	//タイマーが指定した時間になったら回避終了
	if (*avoidTimer.get() >= avoidTime) {
		isAvoid = false;
	}
}

void Player::Jump()
{
	//ジャンプ回数が連続ジャンプ可能回数を超えていたら抜ける
	if (jumpCount >= jumpMaxNum) { return; }
	//ジャンプ入力がなければ抜ける
	if (!GameInputManager::TriggerInputAction(GameInputManager::Jump)) { return; }
	//持久力がジャンプで使用する値以下なら抜ける
	if (endurance < jumpUseEndurance) { return; }
	UseEndurance(jumpUseEndurance, 30, true); //持久力を使用

	onGround = false;
	isInputJump = true;
	velocity.y = jumpPower;
	fallSpeed = 0;
	jumpCount++; //ジャンプ回数を増やす
}

void Player::BlinkStart()
{
	//ブリンク開始可能でなければ抜ける
	if (!isBlinkStart) { return; }
	//ジャンプ中でなければ抜ける
	if (!(jumpCount >= 1)) { return; }
	//ブリンク入力がなければ抜ける
	if (!GameInputManager::TriggerInputAction(GameInputManager::Avoid_Blink_Dash)) { return; }
	//持久力がブリンクで使用する値以下なら抜ける	
	if (endurance < blinkUseEndurance) { return; }
	UseEndurance(blinkUseEndurance, 30, true); //持久力を使用

	//ブリンクするベクトルを求める(現在向いている方向)
	const float rotaRadian = XMConvertToRadians(rota.y - 90);
	blinkVec.x = cosf(rotaRadian);
	blinkVec.z = -sinf(rotaRadian);

	//落下速度を0にする
	velocity.y = 0;

	blinkTimer->Reset();
	isBlink = true;
	isBlinkStart = false;
}

void Player::Blink()
{
	//タイマー更新
	const float blinkTime = 30;
	blinkTimer->Update();
	const float time = *blinkTimer.get() / blinkTime;

	const float power = Easing::OutCirc(20, 1, time);

	velocity = blinkVec.normalize() * power;

	//タイマーが指定した時間になったらブリンク終了
	if (*blinkTimer.get() >= blinkTime) {
		isBlink = false;
	}
}

void Player::Attack()
{
	if (!isAttack) {
		//入力で攻撃をセット
		if (GameInputManager::TriggerInputAction(GameInputManager::Attack)) {
			attackAction = std::make_unique<PlayerSwordAttack1>(object.get());
			if (!attackAction->NextAttack(endurance)) { return; }

			UseEndurance(attackAction->GetUseEndranceNum(), 30, true);
			isAttack = true;
		}
	}
	else {
		//次の攻撃を入力可能なら
		if (attackAction->GetIsNextAttackInput()) {
			//入力で攻撃をセット
			if (GameInputManager::TriggerInputAction(GameInputManager::Attack)) {
				if (!attackAction->NextAttack(endurance)) { return; }

				UseEndurance(attackAction->GetUseEndranceNum(), 30, true);
			}
		}
	}
}

void Player::HealHPMove()
{
	if (!isHeal) { return; }

	const float healTime = 10;
	healTimer->Update();
	const float time = *healTimer.get() / healTime;
	HP = (int)Easing::Lerp((float)healBeforeHP, (float)healAfterHP, time);
	hpGauge->ChangeLength(HP, false);

	if (*healTimer.get() >= healTime) {
		isHeal = false;
	}
}

void Player::UseEndurance(const int enduranceUseNum, const int enduranceRecoveryStartTime, bool isDecreaseDiffMode)
{
	//持久力を減らす
	endurance -= enduranceUseNum;
	endurance = max(endurance, 0);
	enduranceGauge->ChangeLength(endurance, isDecreaseDiffMode);

	//回復開始までにかかる時間をセット
	*enduranceRecoveryStartTimer.get() = enduranceRecoveryStartTime;
}

void Player::EnduranceRecovery()
{
	//持久力が最大なら抜ける
	if (endurance >= maxEndurance) { return; }


	//タイマーが0になったら持久力を回復していく
	if (*enduranceRecoveryStartTimer.get() <= 0) {
		endurance++;
		endurance = min(endurance, maxEndurance);
		enduranceGauge->ChangeLength(endurance, false);
	}
	//それ以外ならタイマー更新
	else {
		*enduranceRecoveryStartTimer.get() -= 1;
	}
}

void Player::KnockbackStart(const Vector3& subjectPos, int power)
{
	//攻撃対象と自分のベクトルを算出
	knockbackVec = pos - subjectPos;
	knockbackVec.y = 0;

	//ノックバックの強さをセット(仮で食らったダメージ量 / 10)
	knockbackPower = (float)power / 10;

	//移動スピードを0にしておく
	moveSpeed = 0;

	isKnockback = true;
}

void Player::Knockback()
{
	//ノックバック
	velocity = knockbackVec.normalize() * knockbackPower;

	//ノックバックを弱くしていく
	knockbackPower -= 0.05f;

	//ノックバックによる移動がなくなったらノックバック状態終了
	if (knockbackPower < 0) {
		isKnockback = false;
	}
}
