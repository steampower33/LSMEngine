#include "SphSimCustom.h"

SphSimCustom::SphSimCustom()
{
}

SphSimCustom::~SphSimCustom() {}

void SphSimCustom::Initialize(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList, UINT width, UINT height)
{
	m_position.resize(m_numParticles);
	m_velocity.resize(m_numParticles);
	m_spawnTime.resize(m_numParticles);

	//GenerateEmitterParticles();
	GenerateDamParticles();

	// 디스크립터 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 2 * STRUCTURED_CNT + CONSTANT_CNT;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));
	m_cbvSrvUavHeap->SetName(L"SPH SRV/UAV/CBV Heap");

	// StructuredBuffer 생성
	CreateStructuredBufferWithViews(device, 0, sizeof(XMFLOAT3), m_numParticles, L"Position");
	CreateStructuredBufferWithViews(device, 1, sizeof(XMFLOAT3), m_numParticles, L"PredictedPosition");
	CreateStructuredBufferWithViews(device, 2, sizeof(XMFLOAT3), m_numParticles, L"Velocity");
	CreateStructuredBufferWithViews(device, 3, sizeof(XMFLOAT3), m_numParticles, L"PredictedVelocity");
	CreateStructuredBufferWithViews(device, 4, sizeof(float), m_numParticles, L"Density");
	CreateStructuredBufferWithViews(device, 5, sizeof(float), m_numParticles, L"NearDensity");
	CreateStructuredBufferWithViews(device, 6, sizeof(float), m_numParticles, L"SpawnTime");

	CreateStructuredBufferWithViews(device, 7, sizeof(UINT), m_cellCnt, L"CellCount");
	CreateStructuredBufferWithViews(device, 8, sizeof(UINT) * 2, m_numParticles, L"CellOffset");
	CreateStructuredBufferWithViews(device, 9, sizeof(UINT), m_cellCnt, L"CellStart");
	CreateStructuredBufferWithViews(device, 10, sizeof(UINT), (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX, L"CellStartPartialSum");
	CreateStructuredBufferWithViews(device, 11, sizeof(UINT), m_numParticles, L"CellScatter");

	// Position 초기 상태 -> SRV
	UploadAndCopyData(device, commandList, m_position, sizeof(XMFLOAT3),
		m_positionUploadBuffer, L"PositionUploadBuffer", m_structuredBuffer[m_positionIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// PredictedPosition 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_predictedPositionIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Velocity 초기 상태 -> SRV
	UploadAndCopyData(device, commandList, m_velocity, sizeof(XMFLOAT3),
		m_velocityUploadBuffer, L"VelocityUploadBuffer", m_structuredBuffer[m_velocityIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// PredictedVelocity 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_predictedVelocityIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Density 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_densityIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// NearDensity 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_nearDensityIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// SpawnTime 초기 상태 -> SRV
	UploadAndCopyData(device, commandList, m_spawnTime, sizeof(float),
		m_spawnTimeUploadBuffer, L"SpawnTimeUploadBuffer", m_structuredBuffer[m_spawnTimeIndex],
		D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	// CellCount 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_cellCountIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellOffset 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellStart 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_cellStartIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellStartPartialSum 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// CellScatter 초기 상태 -> UAV
	SetBarrier(commandList, m_structuredBuffer[m_cellScatterIndex],
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	CreateConstantBuffer(device);

	CD3DX12_CPU_DESCRIPTOR_HANDLE currentCpuHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE currentGpuHandle(m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < STRUCTURED_CNT; i++)
	{
		m_structuredBufferSrvCpuHandle[i] = currentCpuHandle;
		currentCpuHandle.Offset(1, m_cbvSrvUavSize);
		m_structuredBufferSrvGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	}

	for (UINT i = 0; i < STRUCTURED_CNT; i++)
	{
		m_structuredBufferUavCpuHandle[i] = currentCpuHandle;
		currentCpuHandle.Offset(1, m_cbvSrvUavSize);
		m_structuredBufferUavGpuHandle[i] = currentGpuHandle;
		currentGpuHandle.Offset(1, m_cbvSrvUavSize);
	}
	m_simParamsCbvGpuHandle = currentGpuHandle;
}

void SphSimCustom::GenerateEmitterParticles()
{
	XMFLOAT3 centerPos = { 0.0f, m_maxBoundsY * 1.5f, 0.0f };

	const UINT num1 = 8, num2 = 16, num3 = 24, num4 = 32;
	const float radius1 = m_dp * 3.0f;
	const float radius2 = m_dp * 6.0f;
	const float radius3 = m_dp * 9.0f;
	const float radius4 = m_dp * 12.0f;
	const UINT batchSize = 1 + num1 + num2 + num3 + num4;

	for (UINT i = 0; i < m_numParticles; ++i)
	{
		UINT groupIdx = i / batchSize;
		UINT subIdx = i % batchSize;

		m_spawnTime[i] = groupIdx * m_simParamsData.deltaTime * 7.0f;

		if (subIdx == 0) {
			m_position[i] = centerPos;
		}
		else if (subIdx < 1 + num1)
		{
			int idx = subIdx - 1;
			float angle = idx * (2 * XM_PI / num1);
			m_position[i] = {
			  centerPos.x,
			  centerPos.y + radius1 * cosf(angle),
			  centerPos.z + radius1 * sinf(angle)
			};
		}
		else if (subIdx < 1 + num1 + num2)
		{
			int idx = subIdx - 1 - num1;
			float angle = idx * (2 * XM_PI / num2);
			m_position[i] = {
			  centerPos.x,
			  centerPos.y + radius2 * cosf(angle),
			  centerPos.z + radius2 * sinf(angle)
			};
		}
		else if (subIdx < 1 + num1 + num2 + num3)
		{
			int idx = subIdx - 1 - num1 - num2;
			float angle = idx * (2 * XM_PI / num3);
			m_position[i] = {
			  centerPos.x,
			  centerPos.y + radius3 * cosf(angle),
			  centerPos.z + radius3 * sinf(angle)
			};
		}
		else
		{
			int idx = subIdx - 1 - num1 - num2 - num3;
			float angle = idx * (2 * XM_PI / num4);
			m_position[i] = {
			  centerPos.x,
			  centerPos.y + radius4 * cosf(angle),
			  centerPos.z + radius4 * sinf(angle)
			};
		}


		XMStoreFloat3(&m_velocity[i], XMVector3Normalize(XMVECTOR{ -1.0f, -0.5f, 0.0f }) * 5.0f);
	}
}

void SphSimCustom::GenerateDamParticles()
{
	float midX = (m_maxBoundsX + -m_maxBoundsX) * 0.5f;
	float midY = (m_maxBoundsY + -m_maxBoundsY) * 0.5f;
	float midZ = (m_maxBoundsZ + -m_maxBoundsZ) * 0.5f;

	float spacingX = m_nX * m_dp;
	float minX = midX - spacingX * 0.5f;
	float maxX = midX + spacingX * 0.5f;

	float spacingY = m_nY * m_dp;
	float minY = midY - spacingY * 0.5f;
	float maxY = midY + spacingY * 0.5f;

	float spacingZ = m_nZ * m_dp;
	float minZ = midZ - spacingZ * 0.5f;
	float maxZ = midZ + spacingZ * 0.5f;

	for (UINT z = 0; z < m_nZ; z++)
	{
		for (UINT y = 0; y < m_nY; y++)
		{
			for (UINT x = 0; x < m_nX; x++)
			{
				UINT index =
					x +
					m_nX * y +
					m_nX * m_nY * z;

				m_position[index].x = -m_maxBoundsX + m_dp * 10.0f + m_dp * x;
				m_position[index].y = (-m_maxBoundsY + midY * 0.5f + m_dp * 10.0f) + m_dp * y;
				m_position[index].z = -m_maxBoundsZ + m_dp + m_dp * z;

				m_velocity[index] = XMFLOAT3{ 0.0f, 0.0f, 0.0f };

				m_spawnTime[index] = -1.0f;
			}
		}
	}

	for (UINT z = 0; z < m_nZ; z++)
	{
		for (UINT y = 0; y < m_nY; y++)
		{
			for (UINT x = 0; x < m_nX; x++)
			{
				UINT index = m_nX * m_nY * m_nZ +
					x +
					m_nX * y +
					m_nX * m_nY * z;

				m_position[index].x = m_maxBoundsX - m_dp * 10.0f - m_dp * x;
				m_position[index].y = (-m_maxBoundsY + midY * 0.5f + m_dp * 10.0f) + m_dp * y;
				m_position[index].z = m_maxBoundsZ - m_dp - m_dp * z;

				m_velocity[index] = XMFLOAT3{ 0.0f, 0.0f, 0.0f };

				m_spawnTime[index] = -1.0f;
			}
		}
	}
}

void SphSimCustom::Update(float dt, UINT& forceKey, UINT& reset, shared_ptr<Camera>& camera, bool isPaused)
{
	m_simParamsData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, -m_maxBoundsZ);
	m_simParamsData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, m_maxBoundsZ);
	m_simParamsData.gridDimX = static_cast<UINT>(ceil(m_maxBoundsX * 2.0f / m_smoothingRadius));
	m_simParamsData.gridDimY = static_cast<UINT>(ceil(m_maxBoundsY * 2.0f / m_smoothingRadius));
	m_simParamsData.gridDimZ = static_cast<UINT>(ceil(m_maxBoundsZ * 2.0f / m_smoothingRadius));
	m_simParamsData.smoothingRadius = m_smoothingRadius;
	m_simParamsData.radius = m_radius;

	if (!isPaused)
		m_simParamsData.currentTime += m_simParamsData.deltaTime;

	if (forceKey == 1)
	{
		forceKey = 10;
		m_simParamsData.startTime = m_simParamsData.currentTime;
		m_simParamsData.endTime = m_simParamsData.currentTime + m_simParamsData.duration;
	}
	m_simParamsData.forceKey = forceKey;
	if (reset == 2)
	{
		m_simParamsData.currentTime = 0.0f;
	}

	memcpy(m_simParamsConstantBufferDataBegin, &m_simParamsData, sizeof(m_simParamsData));

	XMMATRIX view = camera->GetViewMatrix();
	XMMATRIX proj = camera->GetProjectionMatrix();
	
	XMStoreFloat4x4(&m_renderParamsData.view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_renderParamsData.proj, XMMatrixTranspose(proj));

	memcpy(m_renderParamsConstantBufferDataBegin, &m_renderParamsData, sizeof(m_renderParamsData));

	XMStoreFloat4x4(&m_computeParamsData.view, XMMatrixTranspose(view));

	XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
	XMStoreFloat4x4(&m_computeParamsData.invProj, XMMatrixTranspose(invProj));

	m_computeParamsData.eyeWorld = camera->GetEyePos();

	memcpy(m_computeParamsConstantBufferDataBegin, &m_computeParamsData, sizeof(m_computeParamsData));
}

void SphSimCustom::Compute(ComPtr<ID3D12GraphicsCommandList>& commandList, UINT& reset)
{
	if (reset == 1)
	{
		reset = 0;

		GenerateDamParticles();

		// Position
		D3D12_SUBRESOURCE_DATA positionData = {};
		positionData.pData = m_position.data();
		positionData.RowPitch = sizeof(XMFLOAT3) * m_numParticles;
		positionData.SlicePitch = sizeof(XMFLOAT3) * m_numParticles;

		SetBarrier(commandList, m_structuredBuffer[m_positionIndex],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(commandList.Get(), m_structuredBuffer[m_positionIndex].Get(), m_positionUploadBuffer.Get(), 0, 0, 1, &positionData);

		SetBarrier(commandList, m_structuredBuffer[m_positionIndex],
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Velocity
		D3D12_SUBRESOURCE_DATA velocityData = {};
		velocityData.pData = m_velocity.data();
		velocityData.RowPitch = sizeof(XMFLOAT3) * m_numParticles;
		velocityData.SlicePitch = sizeof(XMFLOAT3) * m_numParticles;

		SetBarrier(commandList, m_structuredBuffer[m_velocityIndex],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(commandList.Get(), m_structuredBuffer[m_velocityIndex].Get(), m_velocityUploadBuffer.Get(), 0, 0, 1, &velocityData);

		SetBarrier(commandList, m_structuredBuffer[m_velocityIndex],
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// SpawnTime
		D3D12_SUBRESOURCE_DATA spawnTimeData = {};
		spawnTimeData.pData = m_spawnTime.data();
		spawnTimeData.RowPitch = sizeof(float) * m_numParticles;
		spawnTimeData.SlicePitch = sizeof(float) * m_numParticles;

		SetBarrier(commandList, m_structuredBuffer[m_spawnTimeIndex],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(commandList.Get(), m_structuredBuffer[m_spawnTimeIndex].Get(), m_spawnTimeUploadBuffer.Get(), 0, 0, 1, &spawnTimeData);

		SetBarrier(commandList, m_structuredBuffer[m_spawnTimeIndex],
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}
	else if (reset == 2)
	{
		reset = 0;

		GenerateEmitterParticles();

		// Position
		D3D12_SUBRESOURCE_DATA positionData = {};
		positionData.pData = m_position.data();
		positionData.RowPitch = sizeof(XMFLOAT3) * m_numParticles;
		positionData.SlicePitch = sizeof(XMFLOAT3) * m_numParticles;

		SetBarrier(commandList, m_structuredBuffer[m_positionIndex],
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(commandList.Get(), m_structuredBuffer[m_positionIndex].Get(), m_positionUploadBuffer.Get(), 0, 0, 1, &positionData);

		SetBarrier(commandList, m_structuredBuffer[m_positionIndex],
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Velocity
		D3D12_SUBRESOURCE_DATA velocityData = {};
		velocityData.pData = m_velocity.data();
		velocityData.RowPitch = sizeof(XMFLOAT3) * m_numParticles;
		velocityData.SlicePitch = sizeof(XMFLOAT3) * m_numParticles;

		SetBarrier(commandList, m_structuredBuffer[m_velocityIndex],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(commandList.Get(), m_structuredBuffer[m_velocityIndex].Get(), m_velocityUploadBuffer.Get(), 0, 0, 1, &velocityData);

		SetBarrier(commandList, m_structuredBuffer[m_velocityIndex],
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// SpawnTime
		D3D12_SUBRESOURCE_DATA spawnTimeData = {};
		spawnTimeData.pData = m_spawnTime.data();
		spawnTimeData.RowPitch = sizeof(float) * m_numParticles;
		spawnTimeData.SlicePitch = sizeof(float) * m_numParticles;

		SetBarrier(commandList, m_structuredBuffer[m_spawnTimeIndex],
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

		UpdateSubresources(commandList.Get(), m_structuredBuffer[m_spawnTimeIndex].Get(), m_spawnTimeUploadBuffer.Get(), 0, 0, 1, &spawnTimeData);

		SetBarrier(commandList, m_structuredBuffer[m_spawnTimeIndex],
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}

	{
		commandList->SetComputeRootSignature(Graphics::sphComputeRootSignature.Get());

		ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

		commandList->SetComputeRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[0]); // SRV
		commandList->SetComputeRootDescriptorTable(1, m_structuredBufferUavGpuHandle[0]); // UAV
		commandList->SetComputeRootDescriptorTable(2, m_simParamsCbvGpuHandle); // SimParams CBV
	}

	UINT dispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

	// Update External Forces
	{
		commandList->SetPipelineState(Graphics::sphExternalCSPSO.Get());

		commandList->Dispatch(dispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_predictedPositionIndex]); // PredictedPosition : UAV -> UAV
		SetUAVBarrier(commandList, m_structuredBuffer[m_predictedVelocityIndex]); // PredictedVelocity : UAV -> UAV
	}

	// Clear CellCount
	{
		commandList->SetPipelineState(Graphics::sphClearCountCellCSPSO.Get());

		UINT clearDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;
		commandList->Dispatch(clearDispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_cellCountIndex]); // CellCount : UAV -> UAV
	}

	// CellCount
	{
		commandList->SetPipelineState(Graphics::sphCountCellCSPSO.Get());

		SetBarrier(commandList, m_structuredBuffer[m_predictedPositionIndex],  // predictedPosition : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		UINT countDispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

		commandList->Dispatch(countDispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_cellCountIndex]); // CellCount
		SetUAVBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex]); // CellOffset
	}

	// Scan
	{
		commandList->SetPipelineState(Graphics::sphCellLocalScanCSPSO.Get());

		SetBarrier(commandList, m_structuredBuffer[m_cellCountIndex],   // CellCount : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		UINT scanDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;

		commandList->Dispatch(scanDispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartIndex]); // LocalScan
		SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex]); // partialSum
	}

	// ScanBlock
	{
		commandList->SetPipelineState(Graphics::sphCellLocalScanBlockCSPSO.Get());

		UINT scanBlockDispatchX = ((m_cellCnt + m_groupSizeX - 1) / m_groupSizeX + m_groupSizeX - 1) / m_groupSizeX;

		commandList->Dispatch(scanBlockDispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex]); // partialSum
	}

	// FinalAddition
	{
		commandList->SetPipelineState(Graphics::sphCellFinalAdditionCSPSO.Get());

		SetBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex],  // CellStartPartialSum : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		UINT finalAddtionDispatchX = (m_cellCnt + m_groupSizeX - 1) / m_groupSizeX;

		commandList->Dispatch(finalAddtionDispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_cellStartIndex]); // LocalScan : UAV -> UAV
	}

	// CellScatter
	{
		commandList->SetPipelineState(Graphics::sphCellScatterCSPSO.Get());

		SetBarrier(commandList, m_structuredBuffer[m_cellStartIndex],  // LocalScan : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		SetBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex], // CellOffset : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		UINT cellScatterDispatchX = (m_numParticles + m_groupSizeX - 1) / m_groupSizeX;

		commandList->Dispatch(cellScatterDispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_cellScatterIndex]);
	}

	// Density
	{
		commandList->SetPipelineState(Graphics::sphCalcDensityCSPSO.Get());

		SetBarrier(commandList, m_structuredBuffer[m_cellScatterIndex], // CellScatter : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		commandList->Dispatch(dispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_densityIndex]);
		SetUAVBarrier(commandList, m_structuredBuffer[m_nearDensityIndex]);
	}

	// PressureForce, Viscosity
	{
		commandList->SetPipelineState(Graphics::sphCalcPressureForceCSPSO.Get());

		SetBarrier(commandList, m_structuredBuffer[m_predictedVelocityIndex], // PredictedVelocity : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		SetBarrier(commandList, m_structuredBuffer[m_densityIndex], // Density : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		SetBarrier(commandList, m_structuredBuffer[m_nearDensityIndex], // NearDensity : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		SetBarrier(commandList, m_structuredBuffer[m_velocityIndex],    // Velocity : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->Dispatch(dispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_velocityIndex]);

	}

	// 위치 업데이트, 경계조건 처리
	{
		commandList->SetPipelineState(Graphics::sphCSPSO.Get());

		commandList->Dispatch(dispatchX, 1, 1);

		SetUAVBarrier(commandList, m_structuredBuffer[m_positionIndex]);
		SetUAVBarrier(commandList, m_structuredBuffer[m_velocityIndex]);
	}

	{
		SetBarrier(commandList, m_structuredBuffer[m_positionIndex],    // Position : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		SetBarrier(commandList, m_structuredBuffer[m_velocityIndex],    // Velocity : UAV -> SRV
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		SetBarrier(commandList, m_structuredBuffer[m_predictedPositionIndex], // PredictedPosition: SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_predictedVelocityIndex], // PredictedVelocity : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_densityIndex],   // Density: SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_nearDensityIndex],  // NearDensity : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		SetBarrier(commandList, m_structuredBuffer[m_cellCountIndex],    // CellCount : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_cellOffsetIndex],    // CellOffset : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_cellStartIndex],    // CellStart : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_cellStartPartialSumIndex],    // CellStartPartialSum : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		SetBarrier(commandList, m_structuredBuffer[m_cellScatterIndex],    // Scatter : SRV -> UAV
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	}
}

void SphSimCustom::Render(ComPtr<ID3D12GraphicsCommandList>& commandList,
	ComPtr<ID3D12Resource>& globalConstsUploadHeap)
{
	// Particle, DepthMap, Thickness
	{
		SetBarrier(commandList, m_particleRTVBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		SetBarrier(commandList, m_particleDepthOutputBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->SetGraphicsRootSignature(Graphics::sphRenderRootSignature.Get());

		ID3D12DescriptorHeap* ppHeap[] = { m_cbvSrvUavHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

		commandList->SetGraphicsRootDescriptorTable(0, m_structuredBufferSrvGpuHandle[m_positionIndex]); // SRV 0
		commandList->SetGraphicsRootConstantBufferView(2, m_renderParamsConstantBuffer->GetGPUVirtualAddress()); // CBV
		commandList->SetGraphicsRootConstantBufferView(3, m_simParamsConstantBuffer->GetGPUVirtualAddress()); // CBV

		// Render Particle, LinearDepthMap
		commandList->SetPipelineState(Graphics::sphPSO.Get());

		CD3DX12_CPU_DESCRIPTOR_HANDLE particleRTVHandles[2];
		particleRTVHandles[0] = m_particleRTVHeap->GetCPUDescriptorHandleForHeapStart();
		particleRTVHandles[1] = m_particleDepthOutputRTVHeap->GetCPUDescriptorHandleForHeapStart();
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_particleDSVHeap->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(2, particleRTVHandles, FALSE, &dsvHandle);

		const float particleRTVClearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(particleRTVHandles[0], particleRTVClearValue, 0, nullptr);

		const float particleDepthClearValue[] = { 100.0f, 0.0f, 0.0f, 0.0f };
		commandList->ClearRenderTargetView(particleRTVHandles[1], particleDepthClearValue, 0, nullptr);

		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		commandList->DrawInstanced(m_numParticles, 1, 0, 0);

		SetBarrier(commandList, m_particleRTVBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetBarrier(commandList, m_thicknessRTVBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_RENDER_TARGET);

		// Render ThicknessMap
		commandList->SetPipelineState(Graphics::sphThicknessPSO.Get());

		CD3DX12_CPU_DESCRIPTOR_HANDLE thicknessRTVHandle(m_thicknessRTVHeap->GetCPUDescriptorHandleForHeapStart());
		commandList->OMSetRenderTargets(1, &thicknessRTVHandle, FALSE, nullptr);

		const float thicknessRTVClearValue[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		commandList->ClearRenderTargetView(thicknessRTVHandle, thicknessRTVClearValue, 0, nullptr);

		commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
		commandList->DrawInstanced(m_numParticles, 1, 0, 0);

		SetBarrier(commandList, m_thicknessRTVBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetBarrier(commandList, m_particleDepthOutputBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	}


	{

	}

	// SSFR
	{
		commandList->SetComputeRootSignature(Graphics::sphSSFRSignature.Get());

		ID3D12DescriptorHeap* ppHeap[] = { m_renderHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(ppHeap), ppHeap);

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle(m_renderHeap->GetGPUDescriptorHandleForHeapStart());
		CD3DX12_GPU_DESCRIPTOR_HANDLE uavGpuHandle(m_renderHeap->GetGPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * m_renderSRVCnt);
		commandList->SetComputeRootDescriptorTable(0, srvGpuHandle);
		commandList->SetComputeRootDescriptorTable(1, uavGpuHandle);
		CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGPUHandle(m_renderHeap->GetGPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * m_renderCBVIndex);
		commandList->SetComputeRootDescriptorTable(2, cbvGPUHandle);

		UINT dispatchX = (m_width + 15) / 16;
		UINT dispatchY = (m_height + 15) / 16;

		// Smoothing
		commandList->SetPipelineState(Graphics::sphSmoothingCSPSO.Get());

		SetBarrier(commandList, m_smoothedDepthBuffer,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		commandList->Dispatch(dispatchX, dispatchY, 1);

		SetBarrier(commandList, m_particleDepthOutputBuffer,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

		SetUAVBarrier(commandList, m_smoothedDepthBuffer);
		SetBarrier(commandList, m_smoothedDepthBuffer,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

		// Normal
		commandList->SetPipelineState(Graphics::sphNormalCSPSO.Get());

		commandList->Dispatch(dispatchX, dispatchY, 1);

		SetUAVBarrier(commandList, m_sceneRTVBuffer);
	}
}

void SphSimCustom::CreateStructuredBufferWithViews(
	ComPtr<ID3D12Device>& device, UINT bufferIndex, UINT dataSize, UINT dataCnt, wstring dataName)
{
	UINT particleDataSize = static_cast<UINT>(dataSize * dataCnt);

	auto heapPropsDefault = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto bufferDescParticles = CD3DX12_RESOURCE_DESC::Buffer(particleDataSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

	// 버퍼 생성
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsDefault,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescParticles,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_structuredBuffer[bufferIndex])));
	m_structuredBuffer[bufferIndex]->SetName(dataName.c_str());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = dataCnt;
	srvDesc.Buffer.StructureByteStride = dataSize;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	// SRV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * bufferIndex);

	device->CreateShaderResourceView(
		m_structuredBuffer[bufferIndex].Get(),
		&srvDesc, srvHandle
	);

	// UAV 버퍼 설정
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (STRUCTURED_CNT + bufferIndex));

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = dataCnt;
	uavDesc.Buffer.StructureByteStride = dataSize;
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(
		m_structuredBuffer[bufferIndex].Get(),
		nullptr, &uavDesc, uavHandle);
}

template <typename T>
void SphSimCustom::UploadAndCopyData(ComPtr<ID3D12Device> device,
	ComPtr<ID3D12GraphicsCommandList> commandList, vector<T>& data, UINT dataSize, ComPtr<ID3D12Resource>& uploadBuffer, wstring dataName, ComPtr<ID3D12Resource>& destBuffer, D3D12_RESOURCE_STATES destBufferState)
{
	UINT totalDataSize = dataSize * m_numParticles;
	auto heapPropsUpload = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto bufferDescUpload = CD3DX12_RESOURCE_DESC::Buffer(totalDataSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapPropsUpload,
		D3D12_HEAP_FLAG_NONE,
		&bufferDescUpload,
		D3D12_RESOURCE_STATE_GENERIC_READ, // UPLOAD 힙은 GENERIC_READ에서 시작
		nullptr,
		IID_PPV_ARGS(&uploadBuffer)));
	uploadBuffer->SetName(dataName.c_str());

	D3D12_SUBRESOURCE_DATA uploadData = {};
	uploadData.pData = data.data();
	uploadData.RowPitch = totalDataSize;
	uploadData.SlicePitch = totalDataSize;

	// 대상 버퍼 상태 전이: COMMON -> COPY_DEST
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(commandList.Get(), destBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &uploadData);

	// 대상 버퍼 상태 전이: DEST -> destBufferState
	SetBarrier(commandList, destBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST, destBufferState);

}

void SphSimCustom::CreateConstantBuffer(ComPtr<ID3D12Device> device)
{
	// SimParams 
	{
		m_simParamsData.cellCnt = m_cellCnt;
		m_simParamsData.smoothingRadius = m_smoothingRadius;
		m_simParamsData.radius = m_smoothingRadius * 0.5f;
		m_simParamsData.numParticles = m_numParticles;
		m_simParamsData.minBounds = XMFLOAT3(-m_maxBoundsX, -m_maxBoundsY, -m_maxBoundsZ);
		m_simParamsData.maxBounds = XMFLOAT3(m_maxBoundsX, m_maxBoundsY, m_maxBoundsZ);
		m_simParamsData.gridDimX = m_gridDimX;
		m_simParamsData.gridDimY = m_gridDimY;
		m_simParamsData.gridDimZ = m_gridDimZ;
		m_simParamsData.radius = m_radius;

		CreateConstUploadBuffer(device, m_simParamsConstantBuffer, m_simParamsData, m_simParamsConstantBufferDataBegin);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_simParamsConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_simParamsConstantBufferSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (2 * STRUCTURED_CNT));
		device->CreateConstantBufferView(
			&cbvDesc,
			cbvHandle
		);
	}

	// RenderParams
	{
		CreateConstUploadBuffer(device, m_renderParamsConstantBuffer, m_renderParamsData, m_renderParamsConstantBufferDataBegin);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_renderParamsConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_renderParamsConstantBufferSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_renderHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (m_renderCBVIndex));
		device->CreateConstantBufferView(
			&cbvDesc,
			cbvHandle
		);
	}

	// ComputeParams
	{
		m_computeParamsData.width = m_width;
		m_computeParamsData.invWidth = 1 / static_cast<float>(m_width);
		m_computeParamsData.height = m_height;
		m_computeParamsData.invHeight = 1 / static_cast<float>(m_height);

		CreateConstUploadBuffer(device, m_computeParamsConstantBuffer, m_computeParamsData, m_computeParamsConstantBufferDataBegin);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_computeParamsConstantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_computeParamsConstantBufferSize;

		CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_renderHeap->GetCPUDescriptorHandleForHeapStart(), m_cbvSrvUavSize * (m_renderCBVIndex + 1));
		device->CreateConstantBufferView(
			&cbvDesc,
			cbvHandle
		);
	}
}

void SphSimCustom::InitializeDesciptorHeaps(ComPtr<ID3D12Device>& device, UINT width, UINT height)
{
	m_rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_cbvSrvUavSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_dsvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	m_width = width;
	m_height = height;

	// Render Heap 
	{
		D3D12_DESCRIPTOR_HEAP_DESC renderHeapDesc = {};
		renderHeapDesc.NumDescriptors = m_renderHeapCnt;
		renderHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		renderHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(device->CreateDescriptorHeap(&renderHeapDesc, IID_PPV_ARGS(&m_renderHeap)));
		m_renderHeap->SetName(L"m_renderHeap");
	}

	// Particle Render Buffer
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_particleRTVHeap)));
		m_particleRTVHeap->SetName(L"m_particleRTVHeap");

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		CreateBuffer(device, m_particleRTVBuffer, L"m_particleRTVBuffer",
			DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(width), static_cast<UINT>(height), 1, 
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			m_particleRTVHeap, 0, clearValue,
			m_renderHeap, m_particleRenderSRVIndex, D3D12_SRV_DIMENSION_TEXTURE2D);

		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_particleDSVHeap)));
		m_particleDSVHeap->SetName(L"m_particleDSVHeap");

		D3D12_CLEAR_VALUE depthClearValue = {};
		depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;
		depthClearValue.DepthStencil.Depth = 1.0f;
		depthClearValue.DepthStencil.Stencil = 0;

		D3D12_RESOURCE_DESC depthStencilDesc = {};
		depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthStencilDesc.Width = static_cast<UINT>(width); // 화면 너비
		depthStencilDesc.Height = static_cast<UINT>(height); // 화면 높이
		depthStencilDesc.DepthOrArraySize = 1;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		ThrowIfFailed(device->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&depthStencilDesc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			&depthClearValue,
			IID_PPV_ARGS(&m_particleDSVBuffer)
		));
		m_particleDSVBuffer->SetName(L"m_particleDSVBuffer");

		// DSV 핸들 생성
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

		device->CreateDepthStencilView(m_particleDSVBuffer.Get(), &dsvDesc, m_particleDSVHeap->GetCPUDescriptorHandleForHeapStart());
	}

	// Thickness Buffer
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_thicknessRTVHeap)));
		m_thicknessRTVHeap->SetName(L"m_thicknessRTVHeap");

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R32_FLOAT;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		CreateBuffer(device, m_thicknessRTVBuffer, L"m_thicknessRTVBuffer",
			DXGI_FORMAT_R32_FLOAT, static_cast<UINT>(width), static_cast<UINT>(height), 1,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			m_thicknessRTVHeap, 0, clearValue,
			m_renderHeap, m_thicknessSRVIndex, D3D12_SRV_DIMENSION_TEXTURE2D, 
			m_renderHeap, m_thicknessUAVIndex, D3D12_UAV_DIMENSION_TEXTURE2D);
	}

	// Particle Depth Output Buffer (R32_FLOAT for custom depth)
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = 1;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_particleDepthOutputRTVHeap)));
		m_particleDepthOutputRTVHeap->SetName(L"m_particleDepthOutputRTVHeap");

		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R32_FLOAT;
		clearValue.Color[0] = 100.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 0.0f;

		CreateBuffer(device, m_particleDepthOutputBuffer, L"m_particleDepthOutputBuffer",
			DXGI_FORMAT_R32_FLOAT, static_cast<UINT>(width), static_cast<UINT>(height), 1,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			m_particleDepthOutputRTVHeap, 0, clearValue,
			m_renderHeap, m_particleDepthOutputSRVIndex, D3D12_SRV_DIMENSION_TEXTURE2D);
	}

	// Smoothed Depth Buffer
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R32_FLOAT;
		clearValue.Color[0] = 0.0f;

		CreateBuffer(device, m_smoothedDepthBuffer, L"m_smoothedDepthBuffer",
			DXGI_FORMAT_R32_FLOAT, static_cast<UINT>(width), static_cast<UINT>(height), 1,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			nullptr, 0, clearValue,
			m_renderHeap, m_smoothedDepthSRVIndex, D3D12_SRV_DIMENSION_TEXTURE2D,
			m_renderHeap, m_smoothedDepthUAVIndex, D3D12_UAV_DIMENSION_TEXTURE2D);
	}

	// Normal Buffer
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 0.0f;

		CreateBuffer(device, m_normalBuffer, L"m_normalBuffer",
			DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(width), static_cast<UINT>(height), 1,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			nullptr, 0, clearValue,
			m_renderHeap, m_normalSRVIndex, D3D12_SRV_DIMENSION_TEXTURE2D,
			m_renderHeap, m_normalUAVIndex, D3D12_UAV_DIMENSION_TEXTURE2D);
	}

	// Scene Buffer
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;

		CreateBuffer(device, m_sceneRTVBuffer, L"m_sceneRTVBuffer",
			DXGI_FORMAT_R8G8B8A8_UNORM, static_cast<UINT>(width), static_cast<UINT>(height), 1,
			D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			nullptr, 0, clearValue,
			m_renderHeap, m_sceneSRVIndex, D3D12_SRV_DIMENSION_TEXTURE2D,
			m_renderHeap, m_sceneUAVIndex, D3D12_UAV_DIMENSION_TEXTURE2D);
	}
}