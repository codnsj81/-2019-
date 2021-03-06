#pragma once

#define DIR_FORWARD				0x01
#define DIR_BACKWARD			0x02
#define DIR_LEFT				0x04
#define DIR_RIGHT				0x08
#define DIR_UP					0x10
#define DIR_DOWN				0x20

// 플레이어충돌
#define COLLIDEY	0 // 충돌 O
#define COLLIDEN	1 // 충돌 X


#include "Object.h"
#include "Camera.h"
#include "../Headers/Include.h"
class CScene;
class CProgressUI;

enum SkillState {SKILL_CHARGING, SKILL_FULL, SKILL_USING};

class CPlayer : public CGameObject
{
protected:

	CShadow*					m_pShadow = NULL;

	CTexture*					m_pNomralTex = NULL;
	CTexture*					m_pSkillTex = NULL;

	XMFLOAT3					m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3					m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	XMFLOAT3					m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	float           			m_fPitch = 0.0f;
	float           			m_fYaw = 0.0f;
	float           			m_fRoll = 0.0f;

	XMFLOAT3					m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3     				m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float           			m_fMaxVelocityXZ = 0.0f;
	float           			m_fMaxVelocityY = 0.0f;
	float           			m_fFriction = 0.0f;

	LPVOID						m_pPlayerUpdatedContext = NULL;
	LPVOID						m_pCameraUpdatedContext = NULL;
	
	CPlayer*					m_pPartner = NULL;
	CCamera						*m_pCamera = NULL;
	CGameObject					*m_pOnStone = NULL;
	bool						m_bInWater = false;
	bool						m_bBackWalking = false;
	float						m_fAttTime = 0;
	int							m_moveState = STATE_GROUND;
	float						m_fTime = 0.f;
	float						m_fPreHeight = 0;
	CWater						**m_ppWaters;
	int							m_nWater;
	int							m_playerKind;
	float						m_ObjectHeight;
	int							m_iJumpnum = 0 ;
	float						m_fSpeed = 3.25f;
	int							m_iSkillGage = 0;
	CScene*						m_pScene = NULL;

	int							m_iAtt = 50;
	float						m_iHP = 100.f;
	XMFLOAT3					m_predictedPos = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3					PointingPos;

	vector<XMFLOAT3>				m_xmNavigationVector;
	CGameObject*				m_NavGuide = NULL;
	CUI*						m_DashEffect = NULL;
	CUI*						m_ProgressUI = NULL;

	int							m_navListSize;
	int							m_navProcess;
	SkillState					m_eSkillState = SKILL_CHARGING;
	float						m_fSkillTime = 0;
	XMFLOAT4X4					m_originmat;
	int							m_navIndex = 0;
public:
	SkillState GetSkillState() { return m_eSkillState; }
	CPlayer();
	virtual ~CPlayer();
	bool GetBackWalking() { return m_bBackWalking; }
	void SetOriginMatrix();
	void Reset();
	void SetProgressUI(CUI* p) { m_ProgressUI = p; }
	void SetNav(CGameObject* nav);
	void SetDE(CUI* nav);
	void NextRoad(float fTime);
	void LoadNavigation();
	vector<XMFLOAT3>* GetNavigationList() { return &m_xmNavigationVector; }
	int GetNavListSize() { return m_navListSize; }
	void Attack();
	bool GetInWater() { return m_bInWater; }
	void PlusNavigationList();
	void SetScene(CScene* p) { m_pScene = p; }
	int	GetAtt() { return m_iAtt; }
	XMFLOAT3 GetPrecdictedPos() { return m_predictedPos; }
	XMFLOAT3 GetPosition() { return(m_xmf3Position); }
	XMFLOAT3 GetLookVector() { return(m_xmf3Look); }
	XMFLOAT3 GetUpVector() { return(m_xmf3Up); }
	XMFLOAT3 GetRightVector() { return(m_xmf3Right); }

	void SetLookVector(XMFLOAT3 xmf3Look);
	void SetUpVector(XMFLOAT3 xmf3Up);
	void SetRightVector(XMFLOAT3 xmf3Right);

	
	int GetMoveState() { return m_moveState; }
	void SetSkillMode();
	int GetSkillGage() { return m_iSkillGage; }
	void UseSkill();
	float	GetHp() { return m_iHP; }
	void Damage(int d); 
	void PlusSkillGage(int d);

	void SetCheatmode(bool b);
	CGameObject* GetNavGuide() { return m_NavGuide; }

	void SetFriction(float fFriction) { m_fFriction = fFriction; }
	void SetGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = xmf3Gravity; }
	void SetMaxVelocityXZ(float fMaxVelocity) { m_fMaxVelocityXZ = fMaxVelocity; }
	void SetMaxVelocityY(float fMaxVelocity) { m_fMaxVelocityY = fMaxVelocity; }
	void SetVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = xmf3Velocity; }
	void SetPosition(const XMFLOAT3& xmf3Position) { Move(XMFLOAT3(xmf3Position.x - m_xmf3Position.x, xmf3Position.y - m_xmf3Position.y, xmf3Position.z - m_xmf3Position.z), false); }
	void SetParter(CPlayer* pPlayer) { m_pPartner = pPlayer; }
	void SetScale(XMFLOAT3& xmf3Scale) { m_xmf3Scale = xmf3Scale; }
	void CollideSide();

	const XMFLOAT3& GetVelocity() const { return(m_xmf3Velocity); }
	float GetYaw() const { return(m_fYaw); }
	float GetPitch() const { return(m_fPitch); }
	float GetRoll() const { return(m_fRoll); }

	CCamera *GetCamera() { return(m_pCamera); }
	void SetCamera(CCamera *pCamera) { m_pCamera = pCamera; }

	void SetWaters(CWater** waters) { m_ppWaters = waters; }
	void SetnWaters(int n) { m_nWater = n; }

	void SetState(int state);
	bool CheckInWater(XMFLOAT3 pos, CHeightMapTerrain* pTerrain);

	void Move(ULONG nDirection, float fDistance, bool bVelocity = false);
	void Move( XMFLOAT3 xmf3Shift, bool bVelocity = false);
	void Move(float fxOffset = 0.0f, float fyOffset = 0.0f, float fzOffset = 0.0f);

	void Dash(float fDistance);
	void Pop(float fDistance);
	void Rotate(float x, float y, float z);

	void SetStun();
	

	void SetFullHP() { m_iHP = 100; }

	void OnObject(float fy , CGameObject* stone);
	void Update(float fTimeElapsed);

	virtual void OnPlayerUpdateCallback(float fTimeElapsed) { }
	void SetPlayerUpdatedContext(LPVOID pContext) { m_pPlayerUpdatedContext = pContext; }

	virtual void OnCameraUpdateCallback(float fTimeElapsed) { }
	void SetCameraUpdatedContext(LPVOID pContext) { m_pCameraUpdatedContext = pContext; }

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);

	CCamera *OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode);

	virtual	void Jump();
	virtual CCamera *ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed) { return(NULL); }
	virtual void OnPrepareRender();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL, bool bPrepre = true, int i =0 );


	int							m_CollideState = COLLIDEN;
	bool						m_bCheatmode = false;


	//
	bool						m_bDamaging = false;
	float						m_fDamagingTime = 0.f;
	bool						m_bDash = false;
	bool						m_bPop = false;
};

class CAirplanePlayer : public CPlayer
{
public:
	CAirplanePlayer(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, void *pContext=NULL);
	virtual ~CAirplanePlayer();

	CGameObject					*m_pMainRotorFrame = NULL;
	CGameObject					*m_pTailRotorFrame = NULL;

private:
	virtual void OnPrepareAnimate();
	virtual void Animate(float fTimeElapsed);

public:
	virtual CCamera *ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed);
	virtual void OnPrepareRender();
};

class CTerrainPlayer : public CPlayer
{
public:
	CTerrainPlayer(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, char* name, int kind, bool banimation, void* pContext=NULL);
	virtual ~CTerrainPlayer();

public:
	virtual CCamera *ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed);

	virtual void OnPlayerUpdateCallback(float fTimeElapsed);
	virtual void OnCameraUpdateCallback(float fTimeElapsed);
};

