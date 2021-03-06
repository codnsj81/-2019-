//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"
#include "Player.h"

#define MAX_LIGHTS						16 

#define POINT_LIGHT						1
#define SPOT_LIGHT						2
#define DIRECTIONAL_LIGHT				3

class CSnake;
class CStartUI;
class CDamageUI;
class CFish;
class CTrap;
class CGameFramework;
class CItemBox;
class CSceneScreen;
class CCloud;
class CMirrorShader;

struct DashInfo
{
	XMFLOAT3		m_pos;
	int		m_rot;
};
struct LIGHT
{
	XMFLOAT4							m_xmf4Ambient;
	XMFLOAT4							m_xmf4Diffuse;
	XMFLOAT4							m_xmf4Specular;
	XMFLOAT3							m_xmf3Position;
	float 								m_fFalloff;
	XMFLOAT3							m_xmf3Direction;
	float 								m_fTheta; //cos(m_fTheta)
	XMFLOAT3							m_xmf3Attenuation;
	float								m_fPhi; //cos(m_fPhi)
	bool								m_bEnable;
	int									m_nType;
	float								m_fRange;
	float								padding;
};										
										
struct LIGHTS							
{										
	LIGHT								m_pLights[MAX_LIGHTS];
	XMFLOAT4							m_xmf4GlobalAmbient;
	int									m_nLights;
};

class CScene
{
public:
    CScene();
    ~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	
	void BuildDefaultLightsAndMaterials();
	void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void BuildClock(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseObjects();
	void Update(float fTime);

	void PlayerAttack();


	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }
	
	void LoadStone(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void LoadTrap(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);

	void LoadGrass(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void LoadTree(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	bool ProcessInput(UCHAR *pKeysBuffer);
    void AnimateObjects(float fTimeElapsed);
    void Render(ID3D12GraphicsCommandList *pd3dCommandList, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle, CCamera *pCamera=NULL);
	void ObjectsCollides();	
	void ObjectsCollides2();
	void ReleaseUploadBuffers();
	CWater** GetWaters() { return m_ppWaters; }

	void Fishing();
	void TimeCount(float time);

	bool bCreatePDUI = false;
	XMFLOAT3 monDUIPos;

	CUI* m_iClockMin = NULL;
	CUI* m_iClockSec1 = NULL;
	CUI* m_iClockSec2 = NULL;
	CCloud* m_pCloud = NULL;

	CUI* m_BloodScreen = NULL;
	void SetBloodScreenState(bool b);

	CGameObject							*m_pBox = NULL;
	CPlayer								*m_pPlayer = NULL;
	CSnake*								m_pSnake = NULL;
	CGameObject*								m_pFish = NULL;
	CMirrorShader* m_pWaterShader = NULL;
	ID3D12Resource* m_pIstage = NULL;

	ID3D12Device*						m_pd3dDevice = NULL;
	ID3D12GraphicsCommandList*			m_pd3dCommandList = NULL;
	CGameObject* m_dashobj;

	void ResetObjects();
	void PlusTreeData();
	void SaveTreeData();

	void PlusTrapData();
	void SaveTrapData();

	void PlusGrassData();
	void SaveGrassData();
	void LoadDash(ID3D12Device * pd3dDevice, ID3D12GraphicsCommandList * pd3dCommandList, int stage = 1);

	void PlusMushroomData();
	void SaveMushroomData();
	void BuildMushroomData(ID3D12Device * pd3dDevice, ID3D12GraphicsCommandList * pd3dCommandList);

	void PlusStoneData();
	void SaveStoneData();


	void PlusDashData();
	void SaveDashData();

	void PlusMonsterData();
	void SaveMonsterData();


	void PlusBoxData();
	void SaveBoxhData();
	void LoadBoxData(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void LoadBoxData2();

	void InitStage();

	void SaveNavigation();
	void BuildTextures(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	void CreateExplosion(const XMFLOAT3& pos);
	void CreateDamageUI(int dam);
	void CreateDamageUIP(const XMFLOAT3 & pos);
	void CreateDamageDEF(const XMFLOAT3& pos);
	void BuildMonsterList(ID3D12Device * pd3dDevice, ID3D12GraphicsCommandList * pd3dCommandList);
	void BuildHouses();

	void RenderStage2(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void RenderStage1(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	
	void SetMainFrame(CGameFramework* frame) { m_MainFramework = frame; }

protected:
	ID3D12RootSignature					*m_pd3dGraphicsRootSignature = NULL;

	static ID3D12DescriptorHeap			*m_pd3dCbvSrvDescriptorHeap;

	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dCbvCPUDescriptorStartHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGPUDescriptorStartHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSrvCPUDescriptorStartHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSrvGPUDescriptorStartHandle;

	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dCbvCPUDescriptorNextHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGPUDescriptorNextHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSrvCPUDescriptorNextHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSrvGPUDescriptorNextHandle;
	


public:
	static void CreateCbvSrvDescriptorHeaps(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nConstantBufferViews, int nShaderResourceViews);

	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nConstantBufferViews, ID3D12Resource *pd3dConstantBuffers, UINT nStride);
	static D3D12_GPU_DESCRIPTOR_HANDLE CreateShaderResourceViews(ID3D12Device *pd3dDevice, CTexture *pTexture, UINT nRootParameter, bool bAutoIncrement);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorNextHandle() { return(m_d3dCbvCPUDescriptorNextHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorNextHandle() { return(m_d3dCbvGPUDescriptorNextHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorNextHandle() { return(m_d3dSrvCPUDescriptorNextHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorNeLxtHandle() { return(m_d3dSrvGPUDescriptorNextHandle); }

	int									m_ItemOrder = 0;
	int									m_Stage = 1;
	XMFLOAT2* m_pStageNum;

	CTexture*							m_ClockTex = NULL;
	CTexture*							m_DamageUITex = NULL;
	CTexture*							m_DamageUITexYellow = NULL;
	CTexture*							m_HitAttackEffectTex = NULL;
	CTexture*							m_ExplosionTex = NULL;
	CShader*							m_pShader = NULL;

	CGameObject* HoneyComb = NULL;
	CGameObject* m_pNet = NULL;
	CGameObject* m_Lamp = NULL;
	CTexture* PotionTex = NULL;

	CGameObject							**m_ppGameObjects = NULL;
	list<CItemBox*>						m_ItemBoxList;
	list<CFish*>						m_FishList;
	list<CTree*>						m_TreeObjectslist;
	list<CGameObject*>					m_Objectslist;
	list<CHoneyComb*>					m_HoneyComblist;
	list<CMushroom*>					m_Mushroomlist;
	list<CDash*>						m_DashList;
	list<CFishtrap*>					m_FishTrapList;
	list<CTrap*>						m_TrapList;
	list<CDamageUI*>					m_DamageUIList;
	list<CFloatingItem*>				m_FloatingItemList;
	list<CGameObject*>					m_HouseList;
	list<CGameObject*>					m_LampList;

	list<CUI*>* m_UIList;

	int									m_nWaters = 0;
	CWater								**m_ppWaters = NULL;


	float								m_fElapsedTime = 0.0f;

	CSkyBox								*m_pSkyBox = NULL;
	CHeightMapTerrain					*m_pTerrain = NULL;
	CHeightMapTerrain*					m_pTerrain2 = NULL;
	CSceneScreen* m_pSceneScreen = NULL;

	LIGHT								*m_pLights = NULL;
	int									m_nLights = 0;

	XMFLOAT4							m_xmf4GlobalAmbient;

	CGameObject** m_HouseObj = NULL;
	ID3D12Resource						*m_pd3dcbLights = NULL;
	LIGHTS								*m_pcbMappedLights = NULL;

	float								m_fStageTime = 0;
		int								m_iStageTime = 0;

	list<XMFLOAT2> TrapDatalist;
	list<XMFLOAT2> MushroomDatalist;
	list<XMFLOAT2> TreeDatalist;
	list<StoneInfo>	StoneDataList;
	list<XMFLOAT2> GrassDataList;;
	list<StoneInfo>	BoxDataList;
	vector<StoneInfo>	MonsterDataList;
	list<DashInfo> DashDataList;

	CGameFramework* m_MainFramework = NULL;

};
