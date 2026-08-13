/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/CRenderItem.DepthStencilTarget.cpp
 *  PURPOSE:
 *
 *****************************************************************************/

#include "StdInc.h"

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::PostConstruct
//
////////////////////////////////////////////////////////////////
void CDepthStencilTargetItem::PostConstruct(CRenderItemManager* pManager, uint uiSizeX, uint uiSizeY, int surfaceFormat, bool bIncludeInMemoryStats)
{
    Super::PostConstruct(pManager, bIncludeInMemoryStats);
    m_uiSizeX = uiSizeX;
    m_uiSizeY = uiSizeY;
    m_eFormat = surfaceFormat;

    // Initial creation of d3d data
    CreateUnderlyingData();
}

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::PreDestruct
//
////////////////////////////////////////////////////////////////
void CDepthStencilTargetItem::PreDestruct()
{
    ReleaseUnderlyingData();
    Super::PreDestruct();
}

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::IsValid
//
////////////////////////////////////////////////////////////////
bool CDepthStencilTargetItem::IsValid()
{
    return m_pD3DDepthStencilSurface != nullptr;
}

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::OnLostDevice
//
////////////////////////////////////////////////////////////////
void CDepthStencilTargetItem::OnLostDevice()
{
    ReleaseUnderlyingData();
    m_uiLastEnsureAttempt = 0;
    m_uiEnsureDelayMs = 0;
}

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::OnResetDevice
//
////////////////////////////////////////////////////////////////
void CDepthStencilTargetItem::OnResetDevice()
{
    CreateUnderlyingData();
    m_uiLastEnsureAttempt = 0;
    m_uiEnsureDelayMs = 0;
}

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::CreateUnderlyingData
//
// Plain hardware Z-stencil surface only. See the class comment in
// CRenderItemManagerInterface.h for why the sampleable path isn't attempted.
//
////////////////////////////////////////////////////////////////
void CDepthStencilTargetItem::CreateUnderlyingData()
{
    assert(!m_pD3DDepthStencilSurface);

    if (FAILED(m_pDevice->CreateDepthStencilSurface(m_uiSizeX, m_uiSizeY, (D3DFORMAT)m_eFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &m_pD3DDepthStencilSurface,
                                                     NULL)))
        return;

    // Update memory used
    m_iMemoryKBUsed = CRenderItemManager::CalcD3DResourceMemoryKBUsage(m_pD3DDepthStencilSurface);

    // Update revision counter
    m_iRevision++;
}

////////////////////////////////////////////////////////////////
//
// CDepthStencilTargetItem::ReleaseUnderlyingData
//
////////////////////////////////////////////////////////////////
void CDepthStencilTargetItem::ReleaseUnderlyingData()
{
    SAFE_RELEASE(m_pD3DDepthStencilSurface)
}
