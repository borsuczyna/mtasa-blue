/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CClientSceneView.h
 *  PURPOSE:     Independently rendered GTA world view
 *
 *****************************************************************************/

// A scene view is itself a drawable render-target texture. Its private depth
// surface is never exposed as a texture, preventing scripts from claiming
// sampleable depth when the device only supplied a regular depth surface.
class CClientSceneView : public CClientRenderTarget
{
    DECLARE_CLASS(CClientSceneView, CClientRenderTarget)
public:
    CClientSceneView(CClientManager* pManager, ElementID ID, CRenderTargetItem* pRenderTargetItem, CDepthStencilTargetItem* pDepthStencilTargetItem)
        : ClassInit(this),
          CClientRenderTarget(pManager, ID, pRenderTargetItem),
          m_pDepthStencilTargetItem(pDepthStencilTargetItem),
          m_fFOV(70.0f),
          m_bRenderRequested(false),
          m_bLastRenderSucceeded(false)
    {
        SetTypeName("dx-sceneview");
    }

    ~CClientSceneView() { SAFE_RELEASE(m_pDepthStencilTargetItem); }

    eClientEntityType GetType() const { return CCLIENTSCENEVIEW; }

    void SetCamera(const CMatrix& matrix, float fFOV)
    {
        m_CameraMatrix = matrix;
        m_fFOV = fFOV;
        m_bCameraConfigured = true;
    }

    void RequestRender() { m_bRenderRequested = true; }
    bool IsRenderRequested() const { return m_bRenderRequested; }
    bool IsCameraConfigured() const { return m_bCameraConfigured; }
    bool ConsumeRenderRequest()
    {
        if (!m_bRenderRequested)
            return false;
        m_bRenderRequested = false;
        return true;
    }

    const CMatrix&           GetCameraMatrix() const { return m_CameraMatrix; }
    float                    GetFOV() const { return m_fFOV; }
    CDepthStencilTargetItem* GetDepthStencilTargetItem() const { return m_pDepthStencilTargetItem; }
    void                     SetLastRenderSucceeded(bool bSucceeded) { m_bLastRenderSucceeded = bSucceeded; }
    bool                     DidLastRenderSucceed() const { return m_bLastRenderSucceeded; }

private:
    CDepthStencilTargetItem* m_pDepthStencilTargetItem;
    CMatrix                  m_CameraMatrix;
    float                    m_fFOV;
    bool                     m_bRenderRequested;
    bool                     m_bLastRenderSucceeded;
    bool                     m_bCameraConfigured = false;
};
