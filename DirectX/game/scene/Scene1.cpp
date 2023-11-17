#include "Scene1.h"
#include "Scene/SceneManager.h"
#include "Input/DirectInput.h"
#include "Input/XInputManager.h"
#include "Object/2d/DebugText.h"
#include "WindowApp.h"
#include <imgui.h>
#include "Object/3d/collider/Collision.h"
#include "Object/3d/collider/MeshCollider.h"
#include "Object/3d/collider/CollisionAttribute.h"
#include "GameHelper.h"
#include "scene/TitleScene.h"
#include "effect/BulletEffect.h"
#include "scene/OnStageTest.h"

using namespace DirectX;

const std::array<XMFLOAT4, 2> COLOR = { XMFLOAT4{ 0.0f,0.0f,0.8f,1.0f } ,{ 0.8f,0.0f,0.0f,1.0f } };

void Scene1::Initialize()
{
	Sprite::LoadTexture("gauge", "Resources/SpriteTexture/gauge.png");

	//地形生成
	field = std::make_unique<Field>();

	player = std::make_unique<Player>();

	GameCamera::SetPlayer(player.get());
	debugCamera = DebugCamera::Create({ 300, 40, 0 });
	camera = std::make_unique<GameCamera>();
	player->SetGameCamera(camera.get());

	//影用光源カメラ初期化
	lightCamera = std::make_unique<LightCamera>(Vector3{ 205, 200, 204 }, Vector3{ 205, 0, 205 });
	const float projectionSize = 1.5f;
	lightCamera->SetProjectionNum({ projectionSize * (float)WindowApp::GetWindowWidth() / 5, projectionSize * (float)WindowApp::GetWindowHeight() / 5 },
		{ -projectionSize * (float)WindowApp::GetWindowWidth() / 5, -projectionSize * (float)WindowApp::GetWindowHeight() / 5 });

	Base3D::SetCamera(camera.get());
	Base3D::SetLightCamera(lightCamera.get());

	/*Sprite::LoadTexture("amm", "Resources/amm.jpg");
	sprite = Sprite::Create("amm", {}, { 1059.0f / 5.0f,1500.0f / 5.0f });
	sprite->SetTexSize({ 1059.0f,1500.0f });
	sprite->Update();*/

	boss = std::make_unique<Boss1>();

	ParticleManager::SetCamera(camera.get());

	stop = false;

	gameoverUi.Initialize();


	actionInputConfig = std::make_unique<ActionInputConfig>();

	BulletEffect::LoadResources();

}

void Scene1::Update()
{
	DirectInput* input = DirectInput::GetInstance();

	//GameHelper::Instance()->SetStop(stop);

	if (!isInputConfigMode) {
		player->Update();
		field->Update(player->GetPosition());
		boss->SetTargetPos(player->GetPosition());
		boss->Update();

		CollisionCheck();

		//カメラ更新
		if (isNormalCamera) {
			camera->Update();
			if (DirectInput::GetInstance()->TriggerKey(DIK_RETURN)) {
				isNormalCamera = !isNormalCamera;
				Base3D::SetCamera(debugCamera.get());
			}
		}
		else {
			debugCamera->Update();
			Base3D::SetCamera(debugCamera.get());
			if (DirectInput::GetInstance()->TriggerKey(DIK_RETURN)) {
				isNormalCamera = !isNormalCamera;
				Base3D::SetCamera(camera.get());
			}
		}
		if (DirectInput::GetInstance()->TriggerKey(DIK_9)) {
			OnStageTestScene* testScene = new OnStageTestScene;
			SceneManager::SetNextScene(testScene);
		}
		//camera->Update();
		lightCamera->Update();

		//デバッグ用シーン切り替え
		if (DirectInput::GetInstance()->ReleaseKey(DIK_1)) {
			TitleScene* titleScene = new TitleScene;
			SceneManager::SetNextScene(titleScene);
		}

		gameoverUi.Update();
		//体力0でゲームオーバー表示
		//デバッグ用ゲームオーバー表示
		if (DirectInput::GetInstance()->TriggerKey(DIK_F4)) { gameoverUi.ResetGameOverUI(); }
		//if (DirectInput::GetInstance()->TriggerKey(DIK_4)) { gameoverUi.StartGameOverUI(); }
		//if (player->GetIsDead() && !gameoverUi.GetIsGameOver()) { gameoverUi.StartGameOverUI(); }

		if (DirectInput::GetInstance()->TriggerKey(DIK_TAB) || XInputManager::GetInstance()->TriggerButton(XInputManager::PAD_START)) {
			isInputConfigMode = true;
			actionInputConfig->Reset();
		}
	}
	else {
		//入力設定更新
		actionInputConfig->Update();

		if (actionInputConfig->GetIsInputConfigEnd()) { isInputConfigMode = false; }
	}
}

void Scene1::Draw(const int _cameraNum)
{
	field->Draw();
	//gobject->ColliderDraw();

	player->Draw();
	boss->Draw();
}

void Scene1::DrawLightView(const int _cameraNum)
{
	player->DrawLightView();
	boss->DrawLightView();
}

void Scene1::NonPostEffectDraw(const int _cameraNum)
{
	//スプライト
	if (_cameraNum == 0) {
		DebugText::GetInstance()->DrawAll();
		//sprite->Draw();
	}

	player->DrawSprite();
	boss->DrawSprite();
	gameoverUi.Draw();

	//入力設定描画
	if (isInputConfigMode) {
		actionInputConfig->Draw();
	}
}

void Scene1::Finalize()
{
}

void Scene1::ImguiDraw()
{
	Vector3 ppos = player->GetPosition();
	XMFLOAT3 cameraPos = {};
	XMFLOAT3 cameraTarget = {};
	if (isNormalCamera) {
		cameraPos = camera->GetEye();
		cameraTarget = camera->GetTarget();
	}
	else {
		cameraPos = debugCamera->GetEye();
		cameraTarget = debugCamera->GetTarget();
	}

	ImGui::Begin("debug imgui");
	ImGui::SetWindowSize(ImVec2(300, 300), ImGuiCond_::ImGuiCond_FirstUseEver);

	ImGui::Text("Camera Pos    [ %f : %f : %f ]", cameraPos.x, cameraPos.y, cameraPos.z);
	ImGui::Text("Camera Target [ %f : %f : %f ]", cameraTarget.x, cameraTarget.y, cameraTarget.z);
	ImGui::Text("Player Pos    [ %f : %f : %f ]", ppos.x, ppos.y, ppos.z);
	if (camera->GetisLockon()) { ImGui::Text("lockon  true"); }
	else { ImGui::Text("lockon  false"); }
	ImGui::Text("Player Boss Length [ %f ]", boss->GetLength());
	ImGui::Text("%d : %d ", player->GetJumpMaxNum(), player->GetJumpCount());

	player->ImguiDraw();

	ImGui::SliderFloat("blend rate", &rate, 0.0f, 1.0f);
	ImGui::Checkbox("stop", &stop);

	ImGui::End();
}

void Scene1::FrameReset()
{
	field->FrameReset();
	player->FrameReset();
	boss->FrameReset();
}

void Scene1::CollisionCheck()
{
#pragma region プレイヤーと敵の衝突判定
	{
		Sphere playerSphere;
		playerSphere.center = { player->GetPosition().x, player->GetPosition().y,player->GetPosition().z, 1.0f };
		playerSphere.radius = player->GetObject3d()->GetScale().x;

		Sphere enemySphere;
		enemySphere.center = { boss->GetCenter()->GetPosition().x, boss->GetCenter()->GetPosition().y, boss->GetCenter()->GetPosition().z, 1.0f };
		enemySphere.radius = boss->GetCenter()->GetScale().x * 5;

		XMVECTOR inter;
		XMVECTOR reject;
		if (Collision::CheckSphere2Sphere(playerSphere, enemySphere, &inter, &reject)) {
			//プレイヤーを押し戻す
			player->PushBack(reject);
		}
	}
#pragma endregion

#pragma region プレイヤーと敵の攻撃の衝突判定
	{
		//プレイヤーが回避またはブリンクをしていなければ衝突判定
		if (!(player->GetIsAvoid() || player->GetIsBlink())) {
			Sphere playerSphere;
			playerSphere.center = { player->GetPosition().x, player->GetPosition().y,player->GetPosition().z, 1.0f };
			playerSphere.radius = player->GetObject3d()->GetScale().x;

			//球とボックス
			if (boss->GetBaseAction()->GetUseCollision() == BaseAction::UseCollision::box) {
				std::vector<Box> bossAttackDatas;
				boss->GetBaseAction()->GetAttackCollisionBox(bossAttackDatas);

				for (auto& i : bossAttackDatas) {
					if (Collision::CheckSphere2Box(playerSphere, i)) {
						player->Damage(10, i.point1);
						camera->ShakeStart(10, 10);
						break;
					}
				}
			}
			//球と球
			else if (boss->GetBaseAction()->GetUseCollision() == BaseAction::UseCollision::sphere) {
				std::vector<Sphere> bossAttackDatas;
				boss->GetBaseAction()->GetAttackCollisionSphere(bossAttackDatas);

				for (auto& i : bossAttackDatas) {
					if (Collision::CheckSphere2Sphere(playerSphere, i)) {
						player->Damage(10, { i.center.m128_f32[0],i.center.m128_f32[1] ,i.center.m128_f32[2] });
						camera->ShakeStart(10, 10);
						break;
					}
				}
			}
			//球とカプセル
			else if (boss->GetBaseAction()->GetUseCollision() == BaseAction::UseCollision::capsule) {
				std::vector<Capsule> bossAttackDatas;
				boss->GetBaseAction()->GetAttackCollisionCapsule(bossAttackDatas);

				for (auto& i : bossAttackDatas) {
					if (Collision::CheckSphereCapsule(playerSphere, i, nullptr)) {
						player->Damage(10, i.startPosition);
						camera->ShakeStart(10, 10);
						break;
					}
				}
			}
		}
	}
#pragma endregion

#pragma region プレイヤーの攻撃と敵の衝突判定
	{
		//プレイヤーの攻撃がある場合のみ判定 
		if (player->GetAttackAction()) {
			Sphere enemySphere;
			enemySphere.center = { boss->GetCenter()->GetPosition().x, boss->GetCenter()->GetPosition().y, boss->GetCenter()->GetPosition().z, 1.0f };
			enemySphere.radius = boss->GetCenter()->GetScale().x * 5;

			Sphere attackSphere;
			attackSphere.center = player->GetAttackAction()->GetAttackCollisionData().center;
			attackSphere.radius = player->GetAttackAction()->GetAttackCollisionData().radius * 2;

			//攻撃が判定を有効にしていたら判定を取る
			if (Collision::CheckSphere2Sphere(enemySphere, attackSphere) && player->GetAttackAction()->GetIsCollisionValid()) {
				//敵にダメージ
				boss->Damage(player->GetAttackAction()->GetAttackCollisionData().power);

				//毎フレーム多段ヒットするのを防ぐため、この攻撃の衝突判定をoffにしておく。
				player->GetAttackAction()->SetIsCollisionValid(false);

				GameHelper::Instance()->SetSlow(0, 20);
			}
		}
	}
#pragma endregion

#pragma region カメラのロックオンターゲット設定
	{
		//カメラがロックオンターゲットを検出している場合のみ判定
		if (camera->GetisLockonStart()) {
			//敵座標
			XMFLOAT2 pos = boss->GetCenter()->GetScreenPosition();

			//敵のスクリーン座標が検出対象範囲内なら処理
			const float targetScreenDistance = 100;
			const bool isInsideTargetScreen = (pos.x <= WindowApp::GetWindowWidth() - targetScreenDistance && pos.x >= targetScreenDistance &&
				pos.y <= WindowApp::GetWindowHeight() - targetScreenDistance && pos.y >= targetScreenDistance);
			if (isInsideTargetScreen) {
				//ロックオン対象を確定させる
				camera->Lockon(boss->GetCenter());
			}
		}
	}
#pragma endregion
}
