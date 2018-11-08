#pragma once
class CGameFramework // ������ ���� �Դϴ�.
{
	// Direct3D ����̽� ����, ����
	// ȭ�� ����� ���� �������� ó��
	//		���� ��ü�� ����, ����, ����� �Է�, ���ϸ��̼� ��
private:
	HINSTANCE m_hInstance;
	HWND m_hwnd;

	int m_nWndClientWidth;
	int m_nWndClientHeight;

	IDXGIFactory4 *m_pdxgiFactory; // DXGI ���丮 �������̽��� ���� ������
	IDXGISwapChain3 *m_pdxgiSwapChain;//���� ü�� �������̽��� ���� ������(���÷��� ����)
	ID3D12Device *m_pd3dDevice;
	//Direct3D ����̽� �������̽��� ���� ������, ���ҽ� ����

	bool m_bMsaa4xEnable = false;
	UINT m_nMsaa4xQualityLevels = 0;
	//MSAA ���� ���ø� Ȱ��ȭ, ���� ���ø� ���� ����

	static const UINT m_nSwapChainBuffers = 2;	//���� ü�� �ĸ� ���� ����
	UINT m_nSwapChainBufferIndex;

	ID3D12Resource *m_ppd3dRenderTargetBuffers[m_nSwapChainBuffers];// ���� Ÿ�� ����,
	ID3D12DescriptorHeap *m_pd3dRtvDescriptorHeap;//������ �� �������̽� ������,
	UINT m_nRtvDescriptorIncrementSize;  //���� Ÿ�� ������ ����

	ID3D12Resource *m_pd3dDepthStencilBuffer;// ����-���ٽ� ����
	ID3D12DescriptorHeap *m_pd3dDsvDescriptorHeap;//������ �� �������̽� ������
	UINT  m_nDsvDescriptorIncrementSize;//���� - ���ٽ� ������ ���� ũ��

	ID3D12CommandQueue *m_pd3dCommandQueue; // ���ť
	ID3D12CommandAllocator *m_pd3dCommandAllocator;// ��� �Ҵ���
	ID3D12GraphicsCommandList *m_pd3dCommandList;// ��� ����Ʈ �������̽� ������

	ID3D12PipelineState *m_pd3dPipelineState;//�׷��Ƚ� ���������� ���� ��ü�� ���� �������̽� ������
	
	ID3D12Fence *m_pd3dFence;// �潺 �������̽� ������
	UINT64 m_nFenceValue;//�潺 ��
	HANDLE m_hFenceEvent;//�̺�Ʈ �ڵ�

#if defined(_DEBUG)
	ID3D12Debug *m_pd3dDebugController;
#endif
	D3D12_VIEWPORT m_d3dViewport;//����Ʈ
	D3D12_RECT m_d3dScissorRect;//���� �簢��

public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	//�����ӿ�ũ�� �ʱ�ȭ�ϴ� �Լ��̴�(�� �����찡 �����Ǹ� ȣ��ȴ�).

	void OnDestroy(); 
	void CreateSwapChain();//���� ü��
	void CreateDirect3DDevice(); //����̽�
	void CreateRtvAndDsvDescriptorHeaps();// ������ ��
	void CreateCommandQueueAndList();// ��� ť/�Ҵ���/����Ʈ

	void BuildObjects();
	void ReleaseObjects();
	//�������� �޽��� ���� ��ü�� �����ϰ� �Ҹ�

	//�����ӿ�ũ�� �ٽ�(����� �Է�, �ִϸ��̼�, ������)�� ����
	void ProcessInput();
	void AnimateObjects();
	void FrameAdvance();

	void WaitForGpuComplete(); //CPU�� GPU ����ȭ


	//�������� �޽���(Ű����, ���콺 �Է�)�� ó��
	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM
		lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM
		lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam,
		LPARAM lParam);

	void CreateRenderTargetView();
	void CreateDepthStencilView();


};

