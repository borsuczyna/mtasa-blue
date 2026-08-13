/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *               (Shared logic for modifications)
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/shared_logic/CClientDepthStencilTarget.h
 *  PURPOSE:
 *
 *****************************************************************************/

// A standalone depth-stencil surface script element. Deliberately extends
// CClientRenderElement directly rather than CClientTexture/CClientMaterial -
// unlike a render target, it isn't drawable via dxDrawImage.
class CClientDepthStencilTarget : public CClientRenderElement
{
    DECLARE_CLASS(CClientDepthStencilTarget, CClientRenderElement)
public:
    CClientDepthStencilTarget(CClientManager* pManager, ElementID ID, CDepthStencilTargetItem* pDepthStencilTargetItem)
        : ClassInit(this), CClientRenderElement(pManager, ID)
    {
        SetTypeName("dx-depthstenciltarget");
        m_pRenderItem = pDepthStencilTargetItem;
    }

    eClientEntityType GetType() const { return CCLIENTDEPTHSTENCILTARGET; }

    // CClientDepthStencilTarget methods
    CDepthStencilTargetItem* GetDepthStencilTargetItem() { return (CDepthStencilTargetItem*)m_pRenderItem; }
};
