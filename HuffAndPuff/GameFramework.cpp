//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"
#include "Player.h"
#include "SoundMgr.h"



CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_nRtvDescriptorIncrementSize = 0;
	m_nDsvDescriptorIncrementSize = 0;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	CSoundMgr::GetInstacne()->LoadSoundFile();
	_tcscpy_s(m_pszFrameRate, _T("HuffAndPuff-"));
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;

	CreateDirect3DDevice();
	CreateCommandQueueAndList();
	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();
	CreateDepthStencilView();

	CoInitialize(NULL);

	BuildObjects();

	return(true);
}

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1 **)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	/*ID3D12Debug *pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;*/
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void **)&m_pdxgiFactory);

 	IDXGIAdapter1 *pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice))) break;
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void **)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice);
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;
	/////////  터지는곳
	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dCommandList);

	hResult = m_pd3dCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dRtvDescriptorHeap);
	m_nRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dDsvDescriptorHeap);
	m_nDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void **)&m_ppd3dSwapChainBackBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
		d3dRtvCPUDescriptorHandle.ptr += m_nRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, d3dDsvCPUDescriptorHandle);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(!bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
	if (m_FLOWSTATE == SCENE_STAGE1 || m_FLOWSTATE == SCENE_STAGE2)
	{
		switch (nMessageID)
		{
			case WM_LBUTTONDOWN :
				::SetCapture(hWnd);
				::GetCursorPos(&m_ptOldCursorPos);
				break;
			case WM_RBUTTONDOWN:
				m_pScene->PlayerAttack();
				break;
			case WM_LBUTTONUP:
			case WM_RBUTTONUP:
				::ReleaseCapture();
				break;
			default:
				break;
		}
	}
	else if (nMessageID == WM_LBUTTONDOWN)
	{
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		ScreenToClient(hWnd, &m_ptOldCursorPos);
		if (m_FLOWSTATE == SCENE_MAIN)
		{
			MouseClickInMain(m_ptOldCursorPos);
		}
		else if (m_FLOWSTATE == SCENE_MANUAL)
			MouseClickInManual(m_ptOldCursorPos);
		else if (m_FLOWSTATE == SCENE_CLEAR)
			MouseClickInClear(m_ptOldCursorPos);
		else if (m_FLOWSTATE == SCENE_OVER || m_FLOWSTATE == SCENE_OVERTIME)
			MouseClickInOver(m_ptOldCursorPos);
	}
	else if (nMessageID == WM_LBUTTONUP)
		::ReleaseCapture();

}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	switch (nMessageID)
	{
		case WM_KEYUP:
			switch (wParam)
			{
				case VK_ESCAPE:
					::PostQuitMessage(0);
					break;
				case VK_RETURN:
					break;
				case VK_F1:
				case VK_F2:
				case VK_F3:
					m_pCamera = m_pPlayer->ChangeCamera((DWORD)(wParam - VK_F1 + 1), m_GameTimer.GetTimeElapsed());
					break;
				case VK_F9:
					ChangeSwapChainState();
					break;
				case '3': // 네이베게이션 저장
					m_pScene->SaveDashData();
					break;
				case 't':
				case 'T':
					m_pPlayer->SetPosition(XMFLOAT3(1202, 63, 681));
					break;
				case 'y':
				case 'Y':
					m_pScene->PlusDashData();
					break;
				case 'Q': // 스킬
					m_pPlayer->UseSkill(); 
					m_pPlayer->SetFullHP();
					break;
				case 'l':
				case 'L': // 스킬게이지 치트
					m_pPlayer->PlusSkillGage(5);
					break;
					
				default:
					break;
			}
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_SPACE:
				m_pPlayer->Jump();
				break;
			default:
				break;
			}
		default:
			break;

	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE)
				m_GameTimer.Stop();
			else
				m_GameTimer.Start();
			break;
		}
		case WM_SIZE:
			break;
		case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
            break;
		case WM_LBUTTONUP:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
			return 3;
        case WM_KEYDOWN:
        case WM_KEYUP:
			OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
	}
	return(0);
}




void CGameFramework::BuildUI()
{
	// SCENE 내 UI

	
	CUI*  BloodScreen = new CDashEffect(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 80,60, m_pCamera->GetRoatMatrix(), L"Model/Textures/DashEffect.tiff");
	BloodScreen->SetWinpos(0, 0);
	(BloodScreen)->bRender = false;
	m_pPlayer->SetDE(BloodScreen);
	m_UIList->emplace_back(BloodScreen);


	m_pControlEffect = new CBackgroundUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 8, 8, m_pCamera->GetRoatMatrix(), L"Model/Textures/controlEffect.tiff");
	m_pControlEffect->bRender = false;
	m_pControlEffect->SetWinpos(0, -2);
	m_UIList->emplace_back(m_pControlEffect);

	CTexture *HPTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0);
	HPTexture->LoadTextureFromFile(m_pd3dDevice, m_pd3dCommandList, L"Model/Textures/HPBar.tiff", 0,false);
	CScene::CreateShaderResourceViews(m_pd3dDevice, HPTexture, 3, false);


	CUI* pTemp = new CImageUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 20, 3, XMFLOAT3(1999, m_pScene->m_pTerrain->GetHeight(1999, 972), 972), L"Model/Textures/DoggyUI.tiff");
	pTemp->SetWinpos(4.6, 9.5);
	m_UIList->emplace_back(pTemp);

	CUI* m_pHPUI = new CHP(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 10.8f, 0.8f, m_pPlayer->GetPosition());
	m_pHPUI->m_pPlayer = m_pPlayer;
	m_pHPUI->SetWinpos(7.8f, 10.f);
	m_pHPUI->bRender = TRUE;
	m_pHPUI->SetTexture(HPTexture);
	m_UIList->emplace_back(m_pHPUI);
	

	HPTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0);
	HPTexture->LoadTextureFromFile(m_pd3dDevice, m_pd3dCommandList, L"Model/Textures/SkillBar.tiff", 0, false);

	CScene::CreateShaderResourceViews(m_pd3dDevice, HPTexture, 3, false);

	m_pHPUI = new CMP(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 10.8f, 0.8f, m_pPlayer->GetPosition());

	m_pHPUI->m_pPlayer = m_pPlayer;
	m_pHPUI->SetTexture(HPTexture);
	m_pHPUI->SetWinpos(7.8f, 9.05f);
	m_UIList->emplace_back(m_pHPUI);


	 pTemp = new CStartUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 10, 4, m_pPlayer->GetPosition(), L"Model/Textures/StartButton.tiff");
	pTemp->SetWinpos(-2.5, 0);
	dynamic_cast<CStartUI*> (pTemp)->bRender = true;
	dynamic_cast<CStartUI*> (pTemp)->Trigger = true;
	pTemp->m_pPlayer = m_pPlayer;
	m_UIList->emplace_back(pTemp);

	pTemp = new CEndUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(),20,17, XMFLOAT3(1235, m_pScene->m_pTerrain->GetHeight(1235, 616), 616), L"Model/Textures/UI_Clear_Stage1.tif", this);
	pTemp->SetWinpos(0, 0);
	pTemp->m_pPlayer = m_pPlayer;
	m_pClearUI = pTemp;
	m_UIList->emplace_back(pTemp);


	pTemp = new CImageUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 25, 2.5f, XMFLOAT3(1999, m_pScene->m_pTerrain->GetHeight(1999, 972), 972), L"Model/Textures/ProcessBar.tiff");
	pTemp->SetWinpos(-6, -5);
	pTemp->m_fWinposz = 10.f;
	m_UIList->emplace_back(pTemp);

	pTemp = new CProgressUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 1, 1, - 4.25f, -5, L"Model/Textures/DoggyUI2.tiff");
	dynamic_cast<CProgressUI*> (pTemp)->SetProgressWidth(8.5f / (m_pPlayer->GetNavListSize() - 1));
	m_pPlayer->SetProgressUI(pTemp);

	m_UIList->emplace_back(pTemp);


	BloodScreen = new CImageUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 400, 290, XMFLOAT3(1999, m_pScene->m_pTerrain->GetHeight(1999, 972), 972), L"Model/Textures/Red.tiff");
	BloodScreen->bRender = false;
	BloodScreen->SetWinpos(-50, 0);
	m_UIList->emplace_back(BloodScreen);
	m_pScene->m_BloodScreen = BloodScreen;


	m_pOverUI = new CBackgroundUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 20, 17, m_pCamera->GetRoatMatrix(), L"Model/Textures/UI_Failed.tiff");
	m_pOverUI->SetWinpos(0, 0);
	(m_pOverUI)->bRender = false;

	CCloud* cloud = new CCloud(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 25,15, m_pPlayer->GetPosition(), L"Model/Textures/Cloud.tiff");
	cloud->bRender = false;
	cloud->SetWinpos(0, 0);
	m_pScene->m_pCloud = cloud;
	m_UIList->emplace_back(cloud);


	//BackgrundUI Build
	int buiCount = 4;
	m_pBackUIArr = new CBackgroundUI* [buiCount];
	CBackgroundUI* bui = new CBackgroundUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 400, 290, m_pCamera->GetRoatMatrix(), L"Model/Textures/UI_MAIN.tiff");
	m_pBackUIArr[0] = bui;

	bui = new CBackgroundUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 230, 200, m_pCamera->GetRoatMatrix(), L"Model/Textures/UI_Manual.tiff");
	m_pBackUIArr[1] = bui;

	bui = new CBackgroundUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 230, 200, m_pCamera->GetRoatMatrix(), L"Model/Textures/UI_Failed.tiff");
	m_pBackUIArr[2] = bui;

	bui = new CBackgroundUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 230, 200, m_pCamera->GetRoatMatrix(), L"Model/Textures/UI_Failed2.tiff");
	m_pBackUIArr[3] = bui;
}

void CGameFramework::SetbReverseControlMode()
{
	m_pControlEffect->bRender = true;
	m_bReverseControl = true;
}

void CGameFramework::OnDestroy()
{
    ReleaseObjects();

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();

	if (m_pd3dFence) m_pd3dFence->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
    if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

#if defined(_DEBUG)
	IDXGIDebug1	*pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void **)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

#define _WITH_TERRAIN_PLAYER

void CGameFramework::BuildPlayers()
{

	m_pPlayer = new CTerrainPlayer(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), "Model/doggy.bin", PLAYER_KIND_DOGGY, true, m_pScene->m_pTerrain);

	//m_pDoggy->SetPosition(XMFLOAT3(1394, m_pScene->m_pTerrain->GetHeight(1394,1363 ), 1363)); //물 확인용
	m_pPlayer->SetHitBox(XMFLOAT3(5.f, 5.f, 5.f));
	m_pPlayer->SetScale(XMFLOAT3(4.f, 4.f, 4.f));
	m_pPlayer->Rotate(0, 80, 0);
	m_pPlayer->SetOriginMatrix();

	m_pPlayer->SetScene(m_pScene);

	m_pScene->m_pPlayer = m_pPlayer = m_pPlayer;
	m_pCamera = m_pPlayer->GetCamera();
	
	CGameObject* Arrow = new CNavUI(m_pd3dDevice, m_pd3dCommandList, m_pScene->GetGraphicsRootSignature(), 4, 4, XMFLOAT3(0, 0, 0), L"Model/Textures/Pointing.tiff");
		//Arrow->Rotate(0, 80, 0);
	m_pPlayer->SetNav(Arrow);


}

void CGameFramework::BuildObjects()
{
		m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

		m_pScene = new CScene();
		if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList);
		m_pScene->SetMainFrame(this);


		BuildPlayers();

		m_UIList = new list<CUI*>();
		m_pScene->m_UIList = m_UIList;
		m_pCamera->m_UIList = m_UIList;

		BuildUI();
		m_pScene->BuildClock(m_pd3dDevice, m_pd3dCommandList);

		m_pScene->m_pPlayer = m_pPlayer;
		m_pScene->m_pd3dDevice = m_pd3dDevice;
		m_pScene->m_pd3dCommandList = m_pd3dCommandList;

		// m_pSceneScreen->SetPosition();

		m_pd3dCommandList->Close();
		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

		WaitForGpuComplete();

		m_pPlayer->SetWaters(m_pScene->GetWaters());
		m_pPlayer->SetnWaters(2);


		if (m_pScene) m_pScene->ReleaseUploadBuffers();
		if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

		TCHAR* pName = _T("LogoBGM");
		//CSoundMgr::GetInstacne()->PlayBGMSound(pName);

		m_GameTimer.Reset();


}

void CGameFramework::ReleaseObjects()
{
	if (m_pPlayer) m_pPlayer->Release();

	if (m_UIList->size() > 0)
		for (auto n : *m_UIList)
			n->Release();
	delete m_UIList;
	m_UIList = NULL;

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) delete m_pScene;
	if (m_pBackUIArr)
	{
		for (int i = 0; i < 1; i++)
			m_pBackUIArr[i]->Release();
		delete m_pBackUIArr;
	}

}



void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene)
		bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene && ( m_FLOWSTATE == SCENE_STAGE2 || m_FLOWSTATE == SCENE_STAGE1))
	{
		bool ismove = false;
		DWORD dwDirection = 0;
		if (!m_bReverseControl)
		{
			if ((pKeysBuffer['W'] & 0xF0) || (pKeysBuffer['w'] & 0xF0)) {
				dwDirection |= DIR_FORWARD;
			}
			if ((pKeysBuffer['S'] & 0xF0) || (pKeysBuffer['s'] & 0xF0)) {
				dwDirection |= DIR_BACKWARD;
			}
			if ((pKeysBuffer['A'] & 0xF0) || (pKeysBuffer['a'] & 0xF0)) {
				dwDirection |= DIR_LEFT;
			}
			if ((pKeysBuffer['D'] & 0xF0) || (pKeysBuffer['d'] & 0xF0)) {
				dwDirection |= DIR_RIGHT;
			}
		}
		else
		{

			if ((pKeysBuffer['S'] & 0xF0) || (pKeysBuffer['s'] & 0xF0)) {
				dwDirection |= DIR_FORWARD;
			}
			if ((pKeysBuffer['W'] & 0xF0) || (pKeysBuffer['w'] & 0xF0)) {
				dwDirection |= DIR_BACKWARD;
			}
			if ((pKeysBuffer['D'] & 0xF0) || (pKeysBuffer['d'] & 0xF0)) {
				dwDirection |= DIR_LEFT;
			}
			if ((pKeysBuffer['A'] & 0xF0) || (pKeysBuffer['a'] & 0xF0)) {
				dwDirection |= DIR_RIGHT;
			}
		}
		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		if (GetCapture() == m_hWnd)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection) {
				m_pPlayer->Move(dwDirection, 3.25f, true);
				AnimateObjects();
			}
		}
	}

		m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();
	m_pScene->AnimateObjects(fTimeElapsed);


	if (m_pPlayer->GetMoveState() != STATE_STUN)
	{
		m_pPlayer->UpdateTransform(NULL);
		m_pPlayer->Animate(fTimeElapsed);
	}
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
	//m_nSwapChainBufferIndex = (m_nSwapChainBufferIndex + 1) % m_nSwapChainBuffers;

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MouseClickInManual(POINT pos)
{
	if (pos.x > 718 && pos.x <780 && pos.y >141 && pos.y<185 )
	{ 
		m_FLOWSTATE = SCENE_MAIN;
		CSoundMgr::GetInstacne()->PlayEffectSound(_T("Button"));
	}
}

void CGameFramework::MouseClickInMain(POINT pos)
{
	if (pos.x > 324 && pos.x < 681 && pos.y>334 && pos.y < 445)
	{
		m_FLOWSTATE = SCENE_STAGE1;
		m_pScene->m_Stage = 1;
		m_pScene->InitStage();
		CSoundMgr::GetInstacne()->StopALL();
		//CSoundMgr::GetInstacne()->PlayBGMSound(_T("InGame"));
		CSoundMgr::GetInstacne()->PlayEffectSound(_T("Button"));
	}
	else if(pos.x >377 && pos.y > 546 && pos.y < 649 && pos.x < 647)
	{
		m_FLOWSTATE = SCENE_MANUAL;
		CSoundMgr::GetInstacne()->PlayEffectSound(_T("Button"));
	}
}
void CGameFramework::MouseClickInClear(POINT pos)
{

	if (pos.y > 493 && pos.y < 590)
	{
		if (pos.x > 532 && pos.x < 737)
			m_FLOWSTATE = SCENE_MAIN;

		else if (pos.x > 254 && pos.x < 469)
		{
			m_FLOWSTATE = SCENE_STAGE2;
			m_pScene->m_Stage = 2;
			CSoundMgr::GetInstacne()->StopALL();
			CSoundMgr::GetInstacne()->PlayBGMSound(_T("InGame"));
		}
		else
			return;

		m_pScene->ResetObjects();
		m_pScene->InitStage();
		m_pClearUI->bRender = false;
		dynamic_cast<CEndUI*>(m_pClearUI)->Trigger = false;
		dynamic_cast<CEndUI*>(m_pClearUI)->bEx = false;
		CSoundMgr::GetInstacne()->PlayEffectSound(_T("Button"));
	}

}

void CGameFramework::MouseClickInOver(POINT pos)
{
	if (pos.y > 493 && pos.y < 590)
	{
		if (pos.x > 532 && pos.x < 737)
			m_FLOWSTATE = SCENE_MAIN;

		else if (pos.x > 254 && pos.x < 469)
		{
			m_FLOWSTATE = SCENE_STAGE1;
		}
		else
			return;

		m_bPlaying = true;
		m_pOverUI->bRender = false;
		m_pScene->ResetObjects();
		CSoundMgr::GetInstacne()->PlayEffectSound(_T("Button"));
		TCHAR* pName = _T("InGame");
		CSoundMgr::GetInstacne()->PlayBGMSound(pName);
	}
}


//#define _WITH_PLAYER_TOP

void CGameFramework::FrameAdvance()
{    

	m_GameTimer.Tick(0.0f);
	ProcessInput();

		AnimateObjects();

		HRESULT hResult = m_pd3dCommandAllocator->Reset();
		hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

		D3D12_RESOURCE_BARRIER d3dResourceBarrier;
		::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
		d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * m_nRtvDescriptorIncrementSize);

		float pfClearColor[4] = { 1.0f, 1.f, 1.f, 1.0f };
		float pfClearColor2[4] = { 0.1f, 0.1f, 0.15f, 1.0f };
		if(m_FLOWSTATE != SCENE_STAGE2) 
			m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);
		else
			m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor2/*Colors::Azure*/, 0, NULL);


		D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);
		
		if (m_FLOWSTATE == SCENE_STAGE1 || m_FLOWSTATE == SCENE_STAGE2)
			m_pScene->Update(m_GameTimer.GetTimeElapsed());
		

		if (m_bPlaying) {

			if (m_pPlayer->GetHp() <= 0)
			{
				m_bPlaying = false;
				m_FLOWSTATE = SCENE_OVER;
				CSoundMgr::GetInstacne()->StopALL();
			}
		}
		if (m_bReverseControl)
		{
			m_fReverseTime += m_GameTimer.GetTimeElapsed();
			m_pControlEffect->Rotate(0, m_fReverseTime * 100, 0);
			if (m_fReverseTime > 5)
			{
				m_pControlEffect->bRender = false;
				m_bReverseControl = false;
				m_fReverseTime = 0;
			}
		}

		m_pScene->Render(m_pd3dCommandList, d3dDsvCPUDescriptorHandle,  m_pCamera);

		switch (m_FLOWSTATE)
		{
		case SCENE_MAIN:
			m_pBackUIArr[0]->MoveToCamera(m_pCamera);
			m_pBackUIArr[0]->Render(m_pd3dCommandList, m_pCamera);
			break;
		case SCENE_MANUAL:
			m_pBackUIArr[0]->MoveToCamera(m_pCamera);
			m_pBackUIArr[0]->Render(m_pd3dCommandList, m_pCamera);
			m_pBackUIArr[1]->MoveToCamera(m_pCamera);
			m_pBackUIArr[1]->Render(m_pd3dCommandList, m_pCamera);
			break;
		case SCENE_OVER:
			m_pBackUIArr[2]->MoveToCamera(m_pCamera);
			m_pBackUIArr[2]->Render(m_pd3dCommandList, m_pCamera);
			break;
		case SCENE_OVERTIME:
			m_pBackUIArr[3]->MoveToCamera(m_pCamera);
			m_pBackUIArr[3]->Render(m_pd3dCommandList, m_pCamera);
			break;
		}

		//m_pSceneScreen->Render(m_pd3dCommandList, m_pCamera);
#ifdef _WITH_PLAYER_TOP
		m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
		//if (m_pDucky) m_pDucky->Render(m_pd3dCommandList, m_pCamera);
		//if (m_pDoggy) m_pDoggy->Render(m_pd3dCommandList, m_pCamera);
		//m_pPlayer->GetNavGuide()->Render(m_pd3dCommandList, m_pCamera);

		//UI렌더
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

		hResult = m_pd3dCommandList->Close();

		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

		WaitForGpuComplete();

		m_pdxgiSwapChain->Present(0, 0);
		m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();
		MoveToNextFrame();

		m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
		size_t nLength = _tcslen(m_pszFrameRate);
		XMFLOAT3 xmf3Position = m_pPlayer->GetPosition();
		XMFLOAT3 xmf3Look = m_pPlayer->GetLookVector();
		XMFLOAT3 xmf3Right = m_pPlayer->GetRightVector();
		_stprintf_s(m_pszFrameRate + nLength, 70 - nLength, _T("(%4f, %4f, %4f)"), xmf3Position.x, xmf3Position.y, xmf3Position.z);
		::SetWindowText(m_hWnd, m_pszFrameRate);


		CSoundMgr::GetInstacne()->Update();

	::SetWindowText(m_hWnd, m_pszFrameRate);

}

