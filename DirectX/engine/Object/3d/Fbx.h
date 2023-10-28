#pragma once
#include "FbxModel.h"
#include "Base3D.h"
#include "InstanceObject.h"

class Camera;
class LightGroup;

class Fbx : public Base3D
{
protected:// エイリアス
	// Microsoft::WRL::を省略
	template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
	// DirectX::を省略
	using XMFLOAT2 = DirectX::XMFLOAT2;
	using XMFLOAT3 = DirectX::XMFLOAT3;
	using XMFLOAT4 = DirectX::XMFLOAT4;
	using XMMATRIX = DirectX::XMMATRIX;

private://構造体宣言

	//定数バッファ用データB0
	struct ConstBufferDataB0
	{
		XMFLOAT4 color; // ビュープロジェクション行列
		XMMATRIX viewproj; // ビュープロジェクション行列
		XMMATRIX world; // ワールド行列
		XMFLOAT3 cameraPos; // カメラ座標（ワールド座標）
		unsigned int isSkinning;//スキニングを行うか
		unsigned int isBloom;//ブルームの有無
		unsigned int isToon;//トゥーンの有無
		unsigned int isOutline;//アウトラインの有無
		unsigned int isLight;//ライトの有無
		XMFLOAT3 outlineColor;//アウトラインの色
	};

	// 定数バッファ用データ構造体B1
	struct ConstBufferDataB1
	{
		XMFLOAT3 baseColor;//ベースカラ―
		float pad1; // パディング
		XMFLOAT3 ambient; // アンビエント係数
		float pad2; // パディング
		XMFLOAT3 diffuse; // ディフューズ係数
		float metalness; // 金属度(0 or 1)
		float specular; // 鏡面反射度
		float roughness; // 粗さ
		float alpha;	// アルファ
		//float pad[3];//パディング
	};

	struct BoneObjectInfo {
		std::string boneName;
		XMMATRIX matWorld;
		std::string instanceName;
	};

public://静的メンバ関数

	/// <summary>
	/// コンストラクタ
	/// </summary>
	/// <returns></returns>
	Fbx() {};

	/// <summary>
	/// デストラクタ
	/// </summary>
	~Fbx();

	/// <summary>
	/// インスタンスの生成
	/// </summary>
	/// <param name="model">モデル</param
	static std::unique_ptr<Fbx> Create(FbxModel* model = nullptr);

public:

	/// <summary>
	/// 生成
	/// </summary>
	void Initialize();

	/// <summary>
	/// 更新
	/// </summary>
	/// <param name="_motionBlendRate1">現在のモデルの比率</param>
	/// <param name="_motionBlendRate2">ブレンドするモデルの比率</param>
	void Update(const float _motionBlendRate1 = 2.0f, const float _motionBlendRate2 = 2.0f);

	/// <summary>
	/// 描画
	/// </summary>
	void Draw(const DrawMode _drawMode = DrawMode::alpha);

	/// <summary>
	/// 描画
	/// </summary>
	void BoneDraw(const DrawMode _drawMode = DrawMode::alpha);

	/// <summary>
	/// マテリアル情報を定数バッファに送る
	/// </summary>
	void TransferMaterial();

	/// <summary>
	/// bone用モデルセット
	/// </summary>
	/// <param name="_boneName">ボーン名</param>
	/// <param name="_modelName">モデル名</param>
	/// <param name="_model">モデル</param>
	/// <param name="_matWorld">ワールド行列</param>
	void SetBoneObject(const std::string& _boneName, const std::string& _modelName,
		Model* _model = nullptr, const XMMATRIX& _matWorld = DirectX::XMMatrixIdentity());

	void FrameReset();

private://メンバ変数

	//パイプライン情報
	static std::vector<GraphicsPipelineManager::DrawSet> pipeline;

	//モデル
	FbxModel* model = nullptr;
	//motionblend用
	FbxModel* motionBlendModel = nullptr;
	//定数バッファ
	ComPtr<ID3D12Resource> constBuffB0;
	// 定数バッファ
	ComPtr<ID3D12Resource> constBuffB1;
	//マテリアルが変化したか
	bool isTransferMaterial = false;
	//ベースカラ―
	XMFLOAT3 baseColor = { 1,1,1 };
	//金属度(0 or 1)
	float metalness = 0.0f;
	//鏡面反射度
	float specular = 0.5f;
	//粗さ
	float roughness = 0.0f;

	//使用アニメーション番号指定
	int useAnimation;

	std::vector<BoneObjectInfo> boneObjectInfo;
	std::unordered_map<std::string, std::unique_ptr<InstanceObject>> boneObject;

public:

	FbxModel* GetModel() { return model; }
	const XMMATRIX& GetMatWorld() { return matWorld; }
	const XMFLOAT3& GetBaseColor() { return baseColor; }
	float GetMetalness() { return metalness; }
	float GetSpecular() { return specular; }
	float GetRoughness() { return roughness; }
	int SetUseAnimation() { return useAnimation; }
	void SetModel(FbxModel* model) { this->model = model; }
	void SetMotionBlendModel(FbxModel* _model) { motionBlendModel = _model; }
	void SetAnimation(bool isAnimation) { model->isAnimation = isAnimation; }
	void SetBaseColor(const XMFLOAT3& baseColor) { 
		this->baseColor = baseColor;
		isTransferMaterial = true;
	}
	void SetMetalness(float metalness) { 
		this->metalness = metalness;
		isTransferMaterial = true;
	}
	void SetSpecular(float specular) { 
		this->specular = specular;
		isTransferMaterial = true;
	}
	void SetRoughness(float roughness) { 
		this->roughness = roughness;
		isTransferMaterial = true;
	}
	void SetUseAnimation(int _useAnimation) { useAnimation = _useAnimation; }
	static void SetPipeline(const std::vector<GraphicsPipelineManager::DrawSet>& _pipeline) { Fbx::pipeline = _pipeline; }
};