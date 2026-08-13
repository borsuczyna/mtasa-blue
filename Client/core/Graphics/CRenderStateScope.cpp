/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        core/Graphics/CRenderStateScope.cpp
 *  PURPOSE:
 *
 *  Multi Theft Auto is available from https://www.multitheftauto.com/
 *
 *****************************************************************************/

#include "StdInc.h"
#include "CRenderStateScope.h"
#include "DXHook/CProxyDirect3DDevice9.h"
#include <game/CCamera.h>
#include <game/CCam.h>
#include <cmath>

namespace
{
    bool IsFiniteVector(const CVector& vec) { return std::isfinite(vec.fX) && std::isfinite(vec.fY) && std::isfinite(vec.fZ); }

    bool IsFiniteCameraMatrix(const CMatrix& matrix)
    {
        return IsFiniteVector(matrix.vPos) && IsFiniteVector(matrix.vFront) && IsFiniteVector(matrix.vUp) && IsFiniteVector(matrix.vRight);
    }
}            // namespace

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::CRenderStateScope
//
// Captures render targets, depth-stencil and viewport unconditionally.
// Camera is not touched unless ApplyCamera() is called.
//
////////////////////////////////////////////////////////////////
CRenderStateScope::CRenderStateScope(IDirect3DDevice9* pDevice)
    : m_pDevice(pDevice),
      m_iNumRenderTargetSlots(0),
      m_bHasSavedDepthStencil(false),
      m_pSavedDepthStencil(nullptr),
      m_bCameraApplied(false),
      m_fSavedCameraFOV(0.0f)
{
    for (auto& pSurface : m_SavedRenderTargets)
        pSurface = nullptr;
    ZeroMemory(&m_SavedViewport, sizeof(m_SavedViewport));

    if (!m_pDevice)
        return;

    // Only capture as many slots as the device actually exposes - calling
    // GetRenderTarget beyond NumSimultaneousRTs is an invalid call on DX9.
    m_iNumRenderTargetSlots = std::max(1, std::min<int>(g_pDeviceState->DeviceCaps.NumSimultaneousRTs, MAX_MRT_RENDER_TARGETS));

    for (int i = 0; i < m_iNumRenderTargetSlots; ++i)
    {
        if (FAILED(m_pDevice->GetRenderTarget(i, &m_SavedRenderTargets[i])))
            m_SavedRenderTargets[i] = nullptr;
    }

    m_bHasSavedDepthStencil = SUCCEEDED(m_pDevice->GetDepthStencilSurface(&m_pSavedDepthStencil));
    if (!m_bHasSavedDepthStencil)
        m_pSavedDepthStencil = nullptr;

    m_pDevice->GetViewport(&m_SavedViewport);
}

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::~CRenderStateScope
//
// Restores everything captured in the constructor, unconditionally.
//
////////////////////////////////////////////////////////////////
CRenderStateScope::~CRenderStateScope()
{
    if (!m_pDevice)
        return;

    // Flush anything drawn while this scope's targets were active, before switching back
    CGraphics::GetSingleton().OnChangingRenderTarget(m_SavedViewport.Width, m_SavedViewport.Height);

    for (int i = 0; i < m_iNumRenderTargetSlots; ++i)
    {
        m_pDevice->SetRenderTarget(i, m_SavedRenderTargets[i]);
        SAFE_RELEASE(m_SavedRenderTargets[i]);
    }

    m_pDevice->SetDepthStencilSurface(m_bHasSavedDepthStencil ? m_pSavedDepthStencil : nullptr);
    SAFE_RELEASE(m_pSavedDepthStencil);

    m_pDevice->SetViewport(&m_SavedViewport);

    if (m_bCameraApplied)
    {
        CCam*    pCam = nullptr;
        CCamera* pCamera = GetActiveGameCam(pCam);
        if (pCamera && pCam)
            ApplyCameraMatrixToGame(pCamera, pCam, m_SavedCameraMatrix, m_fSavedCameraFOV);
    }
}

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::ApplyRenderTargets
//
////////////////////////////////////////////////////////////////
bool CRenderStateScope::ApplyRenderTargets(IDirect3DSurface9* const targets[MAX_MRT_RENDER_TARGETS], IDirect3DSurface9* pDepthStencil, uint uiViewportSizeX,
                                           uint uiViewportSizeY)
{
    if (!m_pDevice || !targets || !targets[0])
        return false;

    CGraphics::GetSingleton().OnChangingRenderTarget(uiViewportSizeX, uiViewportSizeY);

    for (int i = 0; i < m_iNumRenderTargetSlots; ++i)
    {
        if (FAILED(m_pDevice->SetRenderTarget(i, targets[i])))
            return false;
    }

    if (FAILED(m_pDevice->SetDepthStencilSurface(pDepthStencil)))
        return false;

    D3DVIEWPORT9 viewport;
    viewport.X = 0;
    viewport.Y = 0;
    viewport.Width = uiViewportSizeX;
    viewport.Height = uiViewportSizeY;
    viewport.MinZ = 0.0f;
    viewport.MaxZ = 1.0f;
    return SUCCEEDED(m_pDevice->SetViewport(&viewport));
}

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::ApplyViewport
//
////////////////////////////////////////////////////////////////
bool CRenderStateScope::ApplyViewport(const D3DVIEWPORT9& viewport)
{
    if (!m_pDevice)
        return false;

    return SUCCEEDED(m_pDevice->SetViewport(&viewport));
}

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::ApplyCamera
//
// Mirrors CClientCamera::GetGtaMatrix/SetGtaMatrix's exact approach to
// reading/writing the active CCam, since that is the proven-safe way MTA
// already moves the camera (backs the scripting setCameraMatrix function).
//
////////////////////////////////////////////////////////////////
bool CRenderStateScope::ApplyCamera(const CMatrix& matrix, float fFOV)
{
    if (!IsFiniteCameraMatrix(matrix) || !std::isfinite(fFOV) || fFOV <= 0.0f)
        return false;

    CCam*    pCam = nullptr;
    CCamera* pCamera = GetActiveGameCam(pCam);
    if (!pCamera || !pCam)
        return false;

    if (!m_bCameraApplied)
    {
        pCamera->GetMatrix(&m_SavedCameraMatrix);
        m_SavedCameraMatrix.vFront = *pCam->GetFront();
        m_SavedCameraMatrix.vUp = *pCam->GetUp();
        m_SavedCameraMatrix.vPos = *pCam->GetSource();
        m_SavedCameraMatrix.vRight = -m_SavedCameraMatrix.vRight;
        m_SavedCameraMatrix.OrthoNormalize(CMatrix::AXIS_FRONT, CMatrix::AXIS_UP);
        m_fSavedCameraFOV = pCam->GetFOV();
        m_bCameraApplied = true;
    }

    ApplyCameraMatrixToGame(pCamera, pCam, matrix, fFOV);
    return true;
}

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::GetActiveGameCam
//
////////////////////////////////////////////////////////////////
CCamera* CRenderStateScope::GetActiveGameCam(CCam*& outCam)
{
    outCam = nullptr;

    CGame* pGame = CCore::GetSingleton().GetGame();
    if (!pGame)
        return nullptr;

    CCamera* pCamera = pGame->GetCamera();
    if (!pCamera)
        return nullptr;

    outCam = pCamera->GetCam(pCamera->GetActiveCam());
    return pCamera;
}

////////////////////////////////////////////////////////////////
//
// CRenderStateScope::ApplyCameraMatrixToGame
//
////////////////////////////////////////////////////////////////
void CRenderStateScope::ApplyCameraMatrixToGame(CCamera* pCamera, CCam* pCam, const CMatrix& matrix, float fFOV)
{
    CMatrix matNew = matrix;
    matNew.OrthoNormalize(CMatrix::AXIS_FRONT, CMatrix::AXIS_UP);
    matNew.vRight = -matNew.vRight;            // Camera has this the other way round

    pCamera->SetMatrix(&matNew);
    *pCam->GetUp() = matNew.vUp;
    *pCam->GetFront() = matNew.vFront;
    *pCam->GetSource() = matNew.vPos;
    pCam->SetFOV(fFOV);
}
