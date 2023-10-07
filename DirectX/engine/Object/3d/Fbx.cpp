#include "Fbx.h"
#include "Camera/Camera.h"
#include "Light/LightGroup.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using namespace DirectX;

std::vector<GraphicsPipelineManager::DrawSet> Fbx::pipeline;

Fbx::~Fbx()
{
	constBuffB0.Reset();
	constBuffB1.Reset();
}

void Fbx::Initialize()
{
	HRESULT result = S_FALSE;
	// 定数バッファ1の生成
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 	// アップロード可能
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferDataB0) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffB0));
	assert(SUCCEEDED(result));

	//定数バッファSkinの生成
	result = device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),//アップロード可能
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer((sizeof(ConstBufferDataB1) + 0xff) & ~0xff),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constBuffB1));
	assert(SUCCEEDED(result));
}

std::unique_ptr<Fbx> Fbx::Create(FbxModel* model)
{
	// 3Dオブジェクトのインスタンスを生成
	Fbx* instance = new Fbx();

	//初期化
	instance->Initialize();

	//モデルが指定されていればセットする
	if (model) {
		instance->SetModel(model);
	}

	//マテリアル情報の取得
	instance->baseColor = model->GetBaseColor();
	instance->metalness = model->GetMetalness();
	instance->specular = model->GetSpecular();
	instance->roughness = model->GetRoughness();

	instance->TransferMaterial();

	return std::unique_ptr<Fbx>(instance);
}

void Fbx::Update()
{
	HRESULT result;
	XMMATRIX matScale, matRot, matTrans;

	// スケール、回転、平行移動行列の計算
	matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	matRot = XMMatrixIdentity();
	matRot *= XMMatrixRotationZ(XMConvertToRadians(rotation.z));
	matRot *= XMMatrixRotationX(XMConvertToRadians(rotation.x));
	matRot *= XMMatrixRotationY(XMConvertToRadians(rotation.y));
	matTrans = XMMatrixTranslation(position.x, position.y, position.z);

	// ワールド行列の合成
	matWorld = XMMatrixIdentity(); // 変形をリセット
	matWorld *= matScale; // ワールド行列にスケーリングを反映
	matWorld *= matRot; // ワールド行列に回転を反映
	matWorld *= matTrans; // ワールド行列に平行移動を反映

	// 定数バッファ1へデータ転送
	ConstBufferDataB0* constMap = nullptr;
	result = constBuffB0->Map(0, nullptr, (void**)&constMap);
	constMap->color = { 1,1,1,1 };
	if (camera) {
		constMap->viewproj = camera->GetView() * camera->GetProjection();
		constMap->cameraPos = camera->GetEye();
	} else {
		constMap->viewproj = XMMatrixIdentity();
		constMap->cameraPos = {0.0f,0.0f,0.0f };
	}
	constMap->world = matWorld;
	constMap->isSkinning = model->isSkinning;
	constMap->isBloom = isBloom;
	constMap->isToon = isToon;
	constMap->isOutline = isOutline;
	constMap->isLight = isLight;
	constMap->outlineColor = outlineColor;
	constBuffB0->Unmap(0, nullptr);

	if (isTransferMaterial)
	{
		TransferMaterial();
		isTransferMaterial = false;
	}

	model->Update();
}

void Fbx::Draw(const DrawMode _drawMode)
{
	// nullptrチェック
	assert(device);
	assert(ObjectBase::cmdList);

	// モデルの割り当てがなければ描画しない
	if (model == nullptr) {
		return;
	}

	Update();

	int modeNum = int(_drawMode);

	Base3D::Draw(pipeline[modeNum]);

	// 定数バッファビューをセット
	cmdList->SetGraphicsRootConstantBufferView(0, constBuffB0->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, constBuffB1->GetGPUVirtualAddress());

	// ライトの描画
	light->Draw(cmdList, 2);

	// モデル描画
	model->Draw(cmdList);
}

void Fbx::TransferMaterial()
{
	// 定数バッファへデータ転送
	ConstBufferDataB1* constMap = nullptr;
	HRESULT result = constBuffB1->Map(0, nullptr, (void**)&constMap);
	if (SUCCEEDED(result))
	{
		constMap->baseColor = baseColor;
		constMap->ambient = model->GetAmbient();
		constMap->diffuse = model->GetDiffuse();
		constMap->metalness = metalness;
		constMap->specular = specular;
		constMap->roughness = roughness;
		constMap->alpha = model->GetAlpha();
		constBuffB1->Unmap(0, nullptr);
	}
}