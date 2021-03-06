//------------------------------------------------------- ----------------------
// File: Object.h
//-----------------------------------------------------------------------------

#pragma once

#include "Mesh.h"
#include "Camera.h"

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20

class CShader;
class CStandardShader;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define RESOURCE_TEXTURE2D			0x01
#define RESOURCE_TEXTURE2D_ARRAY	0x02	//[]
#define RESOURCE_TEXTURE2DARRAY		0x03
#define RESOURCE_TEXTURE_CUBE		0x04
#define RESOURCE_BUFFER				0x05



struct CB_GAMEOBJECT_INFO
{
	XMFLOAT4X4						m_xmf4x4World;
	UINT							m_nMaterial;
};


struct SRVROOTARGUMENTINFO
{
	int								m_nRootParameterIndex = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dSrvGpuDescriptorHandle;
};


struct StoneInfo
{
	int							m_iType;
	XMFLOAT3					m_pos;
	XMFLOAT3					m_size;

};
class CTexture
{
public:
	CTexture(int nTextureResources = 1, UINT nResourceType = RESOURCE_TEXTURE2D, int nSamplers = 0);
	virtual ~CTexture();

private:
	int								m_nReferences = 0;

	UINT							m_nTextureType = RESOURCE_TEXTURE2D;

	int								m_nTextures = 0;
	ID3D12Resource					**m_ppd3dTextures = NULL;
	ID3D12Resource					**m_ppd3dTextureUploadBuffers;

	int								m_nSamplers = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE		*m_pd3dSamplerGpuDescriptorHandles = NULL;

public:
	SRVROOTARGUMENTINFO				*m_pRootArgumentInfos = NULL;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	void SetRootArgument(int nIndex, UINT nRootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dsrvGpuDescriptorHandle);
	void SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle);

	void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, int nIndex);
	void ReleaseShaderVariables();

	void LoadTextureFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, wchar_t *pszFileName, UINT nIndex, bool bIsDDSFile=true);

	int GetTextures() { return(m_nTextures); }
	ID3D12Resource *GetTexture(int nIndex) { return(m_ppd3dTextures[nIndex]); }
	UINT GetTextureType() { return(m_nTextureType); }

	void ReleaseUploadBuffers();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

class CGameObject;

class CMaterial
{
public:
	CMaterial(int nTextures);
	virtual ~CMaterial();

private:
	int								m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

public:
	CShader							*m_pShader = NULL;

	XMFLOAT4						m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4						m_xmf4EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4AmbientColor = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

	bool m_bShare = false;
	void SetShader(CShader *pShader);
	void SetMaterialType(UINT nType) { m_nType |= nType; }
	void SetTexture(CTexture *pTexture, UINT nTexture = 0);

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList);

	virtual void ReleaseUploadBuffers();

public:
	UINT							m_nType = 0x00;

	float							m_fGlossiness = 0.0f;
	float							m_fSmoothness = 0.0f;
	float							m_fSpecularHighlight = 0.0f;
	float							m_fMetallic = 0.0f;
	float							m_fGlossyReflection = 0.0f;

public:
	int 							m_nTextures = 0;
	_TCHAR							(*m_ppstrTextureNames)[64] = NULL;
	CTexture						**m_ppTextures = NULL; //0:Albedo, 1:Specular, 2:Metallic, 3:Normal, 4:Emission, 5:DetailAlbedo, 6:DetailNormal

	void LoadTextureFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, UINT nType, UINT nRootParameter, _TCHAR *pwstrTextureName, CTexture **ppTexture, CGameObject *pParent, FILE *pInFile, CShader *pShader, bool bDDS = false);

public:
	static CShader					*m_pStandardShader;
	static CShader					*m_pSkinnedAnimationShader;

	static void CMaterial::PrepareShaders(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);

	void SetStandardShader() { CMaterial::SetShader(m_pStandardShader); }
	void SetSkinnedAnimationShader() { CMaterial::SetShader(m_pSkinnedAnimationShader); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define ANIMATION_TYPE_ONCE			0
#define ANIMATION_TYPE_LOOP			1
#define ANIMATION_TYPE_PINGPONG		2

struct CALLBACKKEY
{
   float  							m_fTime = 0.0f;
   void  							*m_pCallbackData = NULL;
};

//#define _WITH_ANIMATION_SRT
#define _WITH_ANIMATION_INTERPOLATION

class CAnimationSet
{
public:
	CAnimationSet();
	~CAnimationSet();

public:
	char							m_pstrName[64];

	float							m_fLength = 0.0f;
	int								m_nFramesPerSecond = 0; //m_fTicksPerSecond

	int								m_nAnimationBoneFrames = 0; 

	int								m_nKeyFrameTransforms = 0;
	float							*m_pfKeyFrameTransformTimes = NULL;
	XMFLOAT4X4						**m_ppxmf4x4KeyFrameTransforms = NULL;

#ifdef _WITH_ANIMATION_SRT
	int								m_nKeyFrameScales = 0;
	float							*m_pfKeyFrameScaleTimes = NULL;
	XMFLOAT3						**m_ppxmf3KeyFrameScales = NULL;
	int								m_nKeyFrameRotations = 0;
	float							*m_pfKeyFrameRotationTimes = NULL;
	XMFLOAT4						**m_ppxmf4KeyFrameRotations = NULL;
	int								m_nKeyFrameTranslations = 0;
	float							*m_pfKeyFrameTranslationTimes = NULL;
	XMFLOAT3						**m_ppxmf3KeyFrameTranslations = NULL;
#endif

	float 							m_fSpeed = 1.0f;
	float 							m_fPosition = 0.0f;
    int 							m_nType = ANIMATION_TYPE_LOOP; //Once, Loop, PingPong

	int								m_nCurrentKey = -1;

	int 							m_nCallbackKeys = 0;
	CALLBACKKEY 					*m_pCallbackKeys = NULL;

public:
	float GetPosition(float fPosition);
	XMFLOAT4X4 GetSRT(int nFrame, float fPosition);

	void SetCallbackKeys(int nCallbackKeys);
	void SetCallbackKey(int nKeyIndex, float fTime, void *pData);

	void *GetCallback(float fPosition) { return(NULL); }
};

class CAnimationTrack
{
public:
	CAnimationTrack() { }
	~CAnimationTrack() { }

public:
    BOOL 							m_bEnable = true;
    float 							m_fSpeed = 1.0f;
    float 							m_fPosition = 0.0f;
    float 							m_fWeight = 1.0f;

    CAnimationSet 					*m_pAnimationSet = NULL;
};

class CAnimationCallbackHandler
{
public:
   virtual void HandleCallback(void *pCallbackData) { }
};

class CAnimationController 
{
public:
	CAnimationController(int nAnimationTracks=1);
	~CAnimationController();

public:
    float 							m_fTime = 0.0f;

	int								m_nAnimationSets = 0;
	CAnimationSet					*m_pAnimationSets = NULL;

	int								m_nAnimationSet = 0;

	int								m_nAnimationBoneFrames = 0; 
	CGameObject						**m_ppAnimationBoneFrameCaches = NULL;

    int 							m_nAnimationTracks = 0;
    CAnimationTrack 				*m_pAnimationTracks = NULL;

	int  				 			m_nAnimationTrack = 0;

    CGameObject						*m_pRootFrame = NULL;

	int								m_iPlayerKind;
	bool							m_bLoop;
	bool							m_bAnimationEnd = false;
public:
	void SetKind(int i) { m_iPlayerKind = i; }
	void SetLoop(bool stop) { m_bLoop = stop; }
	void SetAnimationSet(int nAnimationSet);

	void ResetPosition(int i);
	void SetCallbackKeys(int nAnimationSet, int nCallbackKeys);
	void SetCallbackKey(int nAnimationSet, int nKeyIndex, float fTime, void *pData);

	void AdvanceTime(float fElapsedTime, CAnimationCallbackHandler *pCallbackHandler);

	int GetAnimationSet() { return m_nAnimationSet; }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CGameObject
{
public:
	int								m_nReferences = 0;

public:
	void AddRef();
	void Release();

public:
	CGameObject();
	CGameObject(int nMaterials);
    virtual ~CGameObject();


public:
	char							m_pstrFrameName[64];

	bool							m_bRender = true;
	CMesh							*m_pMesh = NULL;
	bool							m_bCollides = true;
	int								m_nMaterials = 0;
	CMaterial						**m_ppMaterials = NULL;

	XMFLOAT4X4						m_xmf4x4ToParent;
	XMFLOAT4X4						m_xmf4x4World;
	XMFLOAT3						m_Hitbox;

	CGameObject 					*m_pParent = NULL;
	CGameObject 					*m_pChild = NULL;
	CGameObject 					*m_pSibling = NULL;
	bool							m_pShareTexture = false;

	BoundingBox GetBoundingBox();

	void SetTransform(XMFLOAT4X4 mat);
	void SetTexture(CTexture* tex);
	void SetAnimtaionPos(int i);
	void SetAnimationSpeed(float a);
	void SetMesh(CMesh *pMesh);
	void SetShader(CShader *pShader);
	void SetShader(int nMaterial, CShader *pShader);
	void SetMaterial(int nMaterial, CMaterial *pMaterial);
	void SetHitBox(XMFLOAT3 h) { m_Hitbox = h; }
	XMFLOAT3 GetHitBox() { return m_Hitbox; }
	virtual void Update() {}
	void SetChild(CGameObject *pChild, bool bReferenceUpdate=false);

	virtual void BuildMaterials(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) { }

	virtual int getCollision(CPlayer* player, bool physics = true);

	virtual void OnPrepareAnimate() { }
	virtual void Animate(float fTimeElapsed);

	virtual void OnPrepareRender() { }
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL, bool bPrepare = true, int nPipelineState = 0);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World);
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial);
	
	void ChangeTexture(CTexture* tex);
	virtual void ReleaseUploadBuffers();

	int BBCollision(float minX, float maxX, float minY, float maxY, float minZ, float maxZ,
		float minX1, float maxX1, float minY1, float maxY1, float minZ1, float maxZ1);

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();



	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetScale(float x, float y, float z);

	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f);
	void Rotate(XMFLOAT3 *pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4 *pxmf4Quaternion);

	CGameObject *GetParent() { return(m_pParent); }
	void UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent=NULL);
	CGameObject *FindFrame(char *pstrFrameName);

	CTexture *FindReplicatedTexture(_TCHAR *pstrTextureName);
	CTexture *FindRootAndReplicatedTexture(_TCHAR *pstrTextureName, CGameObject *pParent);

	UINT GetMeshType() { return((m_pMesh) ? m_pMesh->GetType() : 0x00); }

	bool m_ismain = false;
	void SetMain(bool b) { m_ismain = b; }
	bool GetMain() { return m_ismain; }

public:
	CAnimationController 			*m_pAnimationController = NULL;

	CGameObject *GetRootSkinnedGameObject();

	void SetAnimationSet(int nAnimationSet);
	int GetAnimationSet() {
		if (m_pAnimationController)
			return m_pAnimationController->GetAnimationSet();
		else
			return -1;
	}
	int GetAnimationSet_child() { 
		if (m_pChild->m_pAnimationController)
			return m_pChild->m_pAnimationController->GetAnimationSet();
		else
			return -1;
	}

	void CacheSkinningBoneFrames(CGameObject *pRootFrame);

	void LoadMaterialsFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CGameObject *pParent, FILE *pInFile, CShader *pShader, bool bDDS = false);
	void LoadAnimationFromFile(FILE *pInFile);

	static CGameObject *LoadFrameHierarchyFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader *pShader, bool bDDS = false);
	static CGameObject *LoadGeometryAndAnimationFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, char *pstrFileName, CShader *pShader, bool bHasAnimation, bool bDDS = false);

	static void PrintFrameInfo(CGameObject *pGameObject, CGameObject *pParent);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CHeightMapTerrain : public CGameObject
{
	
public:
	CHeightMapTerrain(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, LPCTSTR pFileName, int nWidth, int nLength, XMFLOAT3 xmf3Scale, XMFLOAT4 xmf4Color);
	virtual ~CHeightMapTerrain();

private:
	CHeightMapImage				*m_pHeightMapImage;

	int							m_nWidth;
	int							m_nLength;

	XMFLOAT3					m_xmf3Scale;

public:
	float GetHeight(float x, float z, bool bReverseQuad = false) { return(m_pHeightMapImage->GetHeight(x, z, bReverseQuad) * m_xmf3Scale.y); } //World
	XMFLOAT3 GetNormal(float x, float z) { return(m_pHeightMapImage->GetHeightMapNormal(int(x / m_xmf3Scale.x), int(z / m_xmf3Scale.z))); }

	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL);
	int GetHeightMapWidth() { return(m_pHeightMapImage->GetHeightMapWidth()); }
	int GetHeightMapLength() { return(m_pHeightMapImage->GetHeightMapLength()); }
	XMFLOAT3 GetScale() { return(m_xmf3Scale); }
	float GetWidth() { return(m_nWidth * m_xmf3Scale.x); }
	float GetLength() { return(m_nLength * m_xmf3Scale.z); }
};
/////////////////////////////////

class CWater : public CGameObject
{
public:
	CWater(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, int nWidth, int nLength, XMFLOAT3 xmfPosition );
	~CWater();
	void SetCbvGPUDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle) { m_d3dCbvGPUDescriptorHandle = d3dCbvGPUDescriptorHandle; }
	void SetCbvGPUDescriptorHandlePtr(UINT64 nCbvGPUDescriptorHandlePtr) { m_d3dCbvGPUDescriptorHandle.ptr = nCbvGPUDescriptorHandlePtr; }

	virtual void Animate(float fTimeElapsed);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool Cover, int iPipeline=0);
public:
	int							m_nWidth;
	int							m_nLength;

	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dCbvGPUDescriptorHandle;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);
	virtual ~CSkyBox();

	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL);
};

class CTree : public CGameObject {
public:
	CTree();
	~CTree();

	void SetHoneyDrop() { bHoneyDrop = true; }
	bool GetHoneyDrop() { return bHoneyDrop; }

private:
	bool bHoneyDrop = false;
};

class CHoneyComb : public CGameObject
{
public :
	CHoneyComb();
	~CHoneyComb();
	void SetFloorHeight(float p) { floorHeight = p; }
	virtual void Animate(float fTimeElapsed);
	int getCollision(CPlayer * player);
	bool GetbDie() { return m_bDie; }
	void SetFallingSpeed(float f) { m_fallingspeed = f; }
	int GetDamage() { return m_iDamage; }
protected:
	int		m_iDamage  = 6;
	float m_fallingspeed;
	float m_fElapsedTime;
	bool m_bDie = false;
	float floorHeight;
};

class CFloatingItem : public CGameObject
{
public:
	CFloatingItem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	~CFloatingItem();
	bool GetbDie() { return m_bDie; }
	virtual void Animate(float fTimeElapsed);
protected:

	bool m_bDie = false;
	float m_fElapsedTime = 0;

};

class CFishtrap : public CGameObject {
public:
	CFishtrap() {}
	~CFishtrap() {}

public:
	void Pulled(CPlayer* playet);
	virtual int getCollision(CPlayer* player, bool physics = true);
	bool GetSimpleCollision(CGameObject* Fish);
	bool m_bFull = false;
	float m_fTime = 0;
	float m_fOriginY = 0;
};


class CMushroom : public CGameObject
{
public:
	CMushroom();
	~CMushroom();
	bool GetCollided() { return m_bcollided; }
	void SetCollided(bool b) { m_bcollided = b; }

	virtual void Animate(float fTimeElapsed);

protected:
	float m_fTime = 0.f;
	bool m_bcollided = false;
};

class CTrap : public CMushroom
{
public:
	CTrap();
	~CTrap();

	virtual void Animate(float fTimeElapsed);
};


class CDash : public CGameObject
{
public:
	CDash();
	~CDash();
};

class CShadow : public CGameObject
{
public:
	CShadow();
	~CShadow();
	CShadow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nWidth, int nLength, XMFLOAT3 xmfPosition);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, bool bPrepare = true, int nPipelineState = 0);


};

class CItemBox : public CGameObject
{
public:
	CItemBox();
	~CItemBox() {}

	void SetOriginY(float f) { m_fOriginY = f; }
	virtual void Animate(float fTimeElapsed);
	void SetGroup(int i) { iGroup = i; }
	int GetGruop() { return iGroup; }
private:
	int iGroup;
	bool m_bUp = true;
	float m_fOriginY;
	float m_movingHeight;

};
