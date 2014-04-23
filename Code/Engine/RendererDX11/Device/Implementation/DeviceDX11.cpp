
#include <RendererDX11/PCH.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Device/SwapChainDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/RenderTargetConfigDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Context/ContextDX11.h>

#include <Foundation/Logging/Log.h>
#include <System/Window/Window.h>
#include <Foundation/Math/Color.h>

#include <d3d11.h>

ezGALDeviceDX11::ezGALDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDevice(Description),
    m_pDevice(nullptr),
    m_pDXGIFactory(nullptr),
    m_pDXGIAdapter(nullptr),
    m_pDXGIDevice(nullptr),
    m_FeatureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

ezGALDeviceDX11::~ezGALDeviceDX11()
{
}


// Init & shutdown functions

ezResult ezGALDeviceDX11::InitPlatform()
{
  EZ_LOG_BLOCK("ezGALDeviceDX11::InitPlatform()");

  DWORD dwFlags = 0;
  
  if(m_Description.m_bDebugDevice)
    dwFlags |= D3D11_CREATE_DEVICE_DEBUG;

  D3D_FEATURE_LEVEL FeatureLevels[] =
  {
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
    D3D_FEATURE_LEVEL_10_1,
    D3D_FEATURE_LEVEL_10_0,
    D3D_FEATURE_LEVEL_9_3
  };

  ID3D11DeviceContext* pImmediateContext = nullptr;

  // Manually step through feature levels - if a Win 7 system doesn't have the 11.1 runtime installed
  // The create device call will fail even though the 11.0 (or lower) level could've been
  // intialized successfully
  int FeatureLevelIdx = 0;
  for (FeatureLevelIdx = 0; FeatureLevelIdx < EZ_ARRAY_SIZE(FeatureLevels); FeatureLevelIdx++)
  {
    if (SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, dwFlags, &FeatureLevels[FeatureLevelIdx], 1, D3D11_SDK_VERSION, &m_pDevice, &m_FeatureLevel, &pImmediateContext)))
    {
      break;
    }
  }

  // Nothing could be initialized:
  if (pImmediateContext == nullptr)
  {
    ezLog::Error("Couldn't initialize D3D11 device!");
    return EZ_FAILURE;
  }
  else
  {
    const char* FeatureLevelNames[] =
    {
      "11.1",
      "11.0",
      "10.1",
      "10",
      "9.3"
    };

    EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(FeatureLevels) == EZ_ARRAY_SIZE(FeatureLevelNames));

    ezLog::Info("Initialized D3D11 device with feature level %s.", FeatureLevelNames[FeatureLevelIdx]);
  }
  

  // Create primary context object
  m_pPrimaryContext = EZ_DEFAULT_NEW(ezGALContextDX11)(this, pImmediateContext);
  EZ_ASSERT(m_pPrimaryContext != nullptr, "Couldn't create primary context!");

  if(FAILED(m_pDevice->QueryInterface(__uuidof(IDXGIDevice1), (void **)&m_pDXGIDevice)))
  {
    ezLog::Error("Couldn't get the DXGIDevice1 interface of the D3D11 device - this may happen when running on Windows Vista without SP2 installed!");
    return EZ_FAILURE;
  }
      
  if(FAILED(m_pDXGIDevice->GetParent(__uuidof(IDXGIAdapter), (void **)&m_pDXGIAdapter)))
  {
    return EZ_FAILURE;
  }

  if(FAILED(m_pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void **)&m_pDXGIFactory)))
  {
    return EZ_FAILURE;
  }

  // Fill lookup table
  FillFormatLookupTable();

  /// \todo Get features of the device (depending on feature level, CheckFormat* functions etc.)

  return EZ_SUCCESS;
}

ezResult ezGALDeviceDX11::ShutdownPlatform()
{
  EZ_DEFAULT_DELETE(m_pPrimaryContext);

  EZ_GAL_DX11_RELEASE(m_pDevice);
  EZ_GAL_DX11_RELEASE(m_pDXGIFactory);
  EZ_GAL_DX11_RELEASE(m_pDXGIAdapter);
  EZ_GAL_DX11_RELEASE(m_pDXGIDevice);

  return EZ_SUCCESS;
}


// State creation functions

ezGALBlendState* ezGALDeviceDX11::CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description)
{
  return nullptr;
}

void ezGALDeviceDX11::DestroyBlendStatePlatform(ezGALBlendState* pBlendState)
{
}

ezGALDepthStencilState* ezGALDeviceDX11::CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description)
{
  ezGALDepthStencilStateDX11* pDX11DepthStencilState = EZ_DEFAULT_NEW(ezGALDepthStencilStateDX11)(Description);

  if (pDX11DepthStencilState->InitPlatform(this).Succeeded())
  {
    return pDX11DepthStencilState;
  }
  else
  {
    EZ_DEFAULT_DELETE(pDX11DepthStencilState);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState)
{
}

ezGALRasterizerState* ezGALDeviceDX11::CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = EZ_DEFAULT_NEW(ezGALRasterizerStateDX11)(Description);

  if(pDX11RasterizerState->InitPlatform(this).Succeeded())
  {
    return pDX11RasterizerState;
  }
  else
  {
    EZ_DEFAULT_DELETE(pDX11RasterizerState);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState)
{
  ezGALRasterizerStateDX11* pDX11RasterizerState = static_cast<ezGALRasterizerStateDX11*>(pRasterizerState);
  pDX11RasterizerState->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11RasterizerState);
}

ezGALSamplerState* ezGALDeviceDX11::CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description)
{
  ezGALSamplerStateDX11* pDX11SamplerState = EZ_DEFAULT_NEW(ezGALSamplerStateDX11)(Description);

  if (pDX11SamplerState->InitPlatform(this).Succeeded())
  {
    return pDX11SamplerState;
  }
  else
  {
    EZ_DEFAULT_DELETE(pDX11SamplerState);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState)
{
  ezGALSamplerStateDX11* pDX11SamplerState = static_cast<ezGALSamplerStateDX11*>(pSamplerState);
  pDX11SamplerState->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11SamplerState);
}


// Resource creation functions

ezGALShader* ezGALDeviceDX11::CreateShaderPlatform(const ezGALShaderCreationDescription& Description)
{
  ezGALShaderDX11* pShader = EZ_DEFAULT_NEW(ezGALShaderDX11)(Description);

  if(!pShader->InitPlatform(this).Succeeded())
  {
    EZ_DEFAULT_DELETE(pShader);
    return nullptr;
  }

  return pShader;
}

void ezGALDeviceDX11::DestroyShaderPlatform(ezGALShader* pShader)
{
  ezGALShaderDX11* pDX11Shader = static_cast<ezGALShaderDX11*>(pShader);
  pDX11Shader->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11Shader);
}

ezGALBuffer* ezGALDeviceDX11::CreateBufferPlatform(const ezGALBufferCreationDescription& Description, const void* pInitialData)
{
  ezGALBufferDX11* pBuffer = EZ_DEFAULT_NEW(ezGALBufferDX11)(Description);

  if(!pBuffer->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DEFAULT_DELETE(pBuffer);
    return nullptr;
  }

  return pBuffer;
}

void ezGALDeviceDX11::DestroyBufferPlatform(ezGALBuffer* pBuffer)
{
  ezGALBufferDX11* pDX11Buffer = static_cast<ezGALBufferDX11*>(pBuffer);
  pDX11Buffer->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11Buffer);
}

ezGALTexture* ezGALDeviceDX11::CreateTexturePlatform(const ezGALTextureCreationDescription& Description, const ezArrayPtr<ezGALSystemMemoryDescription>* pInitialData)
{
  ezGALTextureDX11* pTexture = EZ_DEFAULT_NEW(ezGALTextureDX11)(Description);

  if(!pTexture->InitPlatform(this, pInitialData).Succeeded())
  {
    EZ_DEFAULT_DELETE(pTexture);
    return nullptr;
  }

  return pTexture;
}

void ezGALDeviceDX11::DestroyTexturePlatform(ezGALTexture* pTexture)
{
  ezGALTextureDX11* pDX11Texture = static_cast<ezGALTextureDX11*>(pTexture);
  pDX11Texture->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11Texture);
}

ezGALResourceView* ezGALDeviceDX11::CreateResourceViewPlatform(const ezGALResourceViewCreationDescription& Description)
{
  ezGALResourceViewDX11* pResourceView = EZ_DEFAULT_NEW(ezGALResourceViewDX11)(Description);

  if(!pResourceView->InitPlatform(this).Succeeded())
  {
    EZ_DEFAULT_DELETE(pResourceView);
    return nullptr;
  }

  return pResourceView;
}

void ezGALDeviceDX11::DestroyResourceViewPlatform(ezGALResourceView* pResourceView)
{
  ezGALResourceViewDX11* pDX11ResourceView = static_cast<ezGALResourceViewDX11*>(pResourceView);
  pDX11ResourceView->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11ResourceView);
}

ezGALRenderTargetView* ezGALDeviceDX11::CreateRenderTargetViewPlatform(const ezGALRenderTargetViewCreationDescription& Description)
{
  ezGALRenderTargetViewDX11* pRTView = EZ_DEFAULT_NEW(ezGALRenderTargetViewDX11)(Description);

  if(!pRTView->InitPlatform(this).Succeeded())
  {
    EZ_DEFAULT_DELETE(pRTView);
    return nullptr;
  }

  return pRTView;
}

void ezGALDeviceDX11::DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView)
{
  ezGALRenderTargetViewDX11* pDX11RenderTargetView = static_cast<ezGALRenderTargetViewDX11*>(pRenderTargetView);
  pDX11RenderTargetView->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pDX11RenderTargetView);
}


// Other rendering creation functions

/// \todo Move the real code creating things to the implementation files (all?)
ezGALSwapChain* ezGALDeviceDX11::CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description)
{
  ezGALSwapChainDX11* pSwapChain = EZ_DEFAULT_NEW(ezGALSwapChainDX11)(Description);

  if (!pSwapChain->InitPlatform(this).Succeeded())
  {
    EZ_DEFAULT_DELETE(pSwapChain);
    return nullptr;
  }

  return pSwapChain;
}

void ezGALDeviceDX11::DestroySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  ezGALSwapChainDX11* pSwapChainDX11 = static_cast<ezGALSwapChainDX11*>(pSwapChain);
  pSwapChainDX11->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pSwapChainDX11);
}

ezGALFence* ezGALDeviceDX11::CreateFencePlatform()
{
  return nullptr;
}

void ezGALDeviceDX11::DestroyFencePlatform(ezGALFence* pFence)
{
}

ezGALQuery* ezGALDeviceDX11::CreateQueryPlatform(const ezGALQueryCreationDescription& Description)
{
  return nullptr;
}

void ezGALDeviceDX11::DestroyQueryPlatform(ezGALQuery* pQuery)
{
}

ezGALRenderTargetConfig* ezGALDeviceDX11::CreateRenderTargetConfigPlatform(const ezGALRenderTargetConfigCreationDescription& Description)
{
  ezGALRenderTargetConfigDX11* pRenderTargetConfig = EZ_DEFAULT_NEW(ezGALRenderTargetConfigDX11)(Description);

  if(pRenderTargetConfig->InitPlatform(this).Succeeded())
  {
    return pRenderTargetConfig;
  }
  else
  {
    EZ_DEFAULT_DELETE(pRenderTargetConfig);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyRenderTargetConfigPlatform(ezGALRenderTargetConfig* pRenderTargetConfig)
{
  ezGALRenderTargetConfigDX11* pRenderTargetConfigDX11 = static_cast<ezGALRenderTargetConfigDX11*>(pRenderTargetConfig);
  pRenderTargetConfigDX11->DeInitPlatform(this);
  EZ_DEFAULT_DELETE(pRenderTargetConfigDX11);
}

ezGALVertexDeclaration* ezGALDeviceDX11::CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description)
{
  ezGALVertexDeclarationDX11* pVertexDeclaration = EZ_DEFAULT_NEW(ezGALVertexDeclarationDX11)(Description);

  if(pVertexDeclaration->InitPlatform(this).Succeeded())
  {
    return pVertexDeclaration;
  }
  else
  {
    EZ_DEFAULT_DELETE(pVertexDeclaration);
    return nullptr;
  }
}

void ezGALDeviceDX11::DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration)
{

  ezGALVertexDeclarationDX11* pVertexDeclarationDX11 = static_cast<ezGALVertexDeclarationDX11*>(pVertexDeclaration);
  pVertexDeclarationDX11->DeInitPlatform(this);

  EZ_DEFAULT_DELETE(pVertexDeclarationDX11);
}


void ezGALDeviceDX11::GetQueryDataPlatform(ezGALQuery* pQuery, ezUInt64* puiRendererdPixels)
{
  *puiRendererdPixels = 42;
}





// Swap chain functions

void ezGALDeviceDX11::PresentPlatform(ezGALSwapChain* pSwapChain)
{
  IDXGISwapChain* pDXGISwapChain = static_cast<ezGALSwapChainDX11*>(pSwapChain)->GetDXSwapChain();

  pDXGISwapChain->Present(pSwapChain->GetDescription().m_bVerticalSynchronization ? 1 : 0, 0);
}

// Misc functions

void ezGALDeviceDX11::BeginFramePlatform()
{
}

void ezGALDeviceDX11::EndFramePlatform()
{
}

void ezGALDeviceDX11::FlushPlatform()
{
  GetPrimaryContext<ezGALContextDX11>()->GetDXContext()->Flush();
}

void ezGALDeviceDX11::FinishPlatform()
{
}

void ezGALDeviceDX11::SetPrimarySwapChainPlatform(ezGALSwapChain* pSwapChain)
{
  // Make window association
  m_pDXGIFactory->MakeWindowAssociation(pSwapChain->GetDescription().m_pWindow->GetNativeWindowHandle(), 0);
}


void ezGALDeviceDX11::FillCapabilitiesPlatform()
{
  switch (m_FeatureLevel)
  {
  case D3D_FEATURE_LEVEL_11_1:
  case D3D_FEATURE_LEVEL_11_0:
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = true;
    m_Capabilities.m_bInstancing = true;
    m_Capabilities.m_b32BitIndices = true;
    m_Capabilities.m_bIndirectDraw = true;
    m_Capabilities.m_bStreamOut = true;
    m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
    m_Capabilities.m_bTextureArrays = true;
    m_Capabilities.m_bCubemapArrays = true;
    m_Capabilities.m_uiMaxTextureDimension = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Capabilities.m_uiMaxCubemapDimension = D3D11_REQ_TEXTURECUBE_DIMENSION;
    m_Capabilities.m_uiMax3DTextureDimension = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Capabilities.m_uiMaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
    m_Capabilities.m_uiMaxRendertargets = D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Capabilities.m_uiUAVCount = (m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 ? 64 : 8);
    m_Capabilities.m_bAlphaToCoverage = true;
    break;

  case D3D_FEATURE_LEVEL_10_1:
  case D3D_FEATURE_LEVEL_10_0:
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = false;
    m_Capabilities.m_bInstancing = true;
    m_Capabilities.m_b32BitIndices = true;
    m_Capabilities.m_bIndirectDraw = false;
    m_Capabilities.m_bStreamOut = true;
    m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
    m_Capabilities.m_bTextureArrays = true;
    m_Capabilities.m_bCubemapArrays = (m_FeatureLevel == D3D_FEATURE_LEVEL_10_1 ? true : false);
    m_Capabilities.m_uiMaxTextureDimension = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
    m_Capabilities.m_uiMaxCubemapDimension = D3D10_REQ_TEXTURECUBE_DIMENSION;
    m_Capabilities.m_uiMax3DTextureDimension = D3D10_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
    m_Capabilities.m_uiMaxAnisotropy = D3D10_REQ_MAXANISOTROPY;
    m_Capabilities.m_uiMaxRendertargets = D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Capabilities.m_uiUAVCount = 0;
    m_Capabilities.m_bAlphaToCoverage = true;
    break;

  case D3D_FEATURE_LEVEL_9_3:
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::VertexShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::HullShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::DomainShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::GeometryShader] = false;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::PixelShader] = true;
    m_Capabilities.m_bShaderStageSupported[ezGALShaderStage::ComputeShader] = false;
    m_Capabilities.m_bInstancing = true;
    m_Capabilities.m_b32BitIndices = true;
    m_Capabilities.m_bIndirectDraw = false;
    m_Capabilities.m_bStreamOut = false;
    m_Capabilities.m_uiMaxConstantBuffers = D3D11_COMMONSHADER_CONSTANT_BUFFER_HW_SLOT_COUNT;
    m_Capabilities.m_bTextureArrays = false;
    m_Capabilities.m_bCubemapArrays = false;
    m_Capabilities.m_uiMaxTextureDimension = D3D_FL9_3_REQ_TEXTURE1D_U_DIMENSION;
    m_Capabilities.m_uiMaxCubemapDimension = D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
    m_Capabilities.m_uiMax3DTextureDimension = 0;
    m_Capabilities.m_uiMaxAnisotropy = 16;
    m_Capabilities.m_uiMaxRendertargets = D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_Capabilities.m_uiUAVCount = 0;
    m_Capabilities.m_bAlphaToCoverage = false;
    break;

  }

}


void ezGALDeviceDX11::FillFormatLookupTable()
{

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32B32_TYPELESS).RT(DXGI_FORMAT_R32G32B32_FLOAT).VA(DXGI_FORMAT_R32G32B32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32G32_TYPELESS).RT(DXGI_FORMAT_R32G32_FLOAT).VA(DXGI_FORMAT_R32G32_FLOAT));

  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RFloat,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R32_TYPELESS).RT(DXGI_FORMAT_R32_FLOAT).VA(DXGI_FORMAT_R32_FLOAT).RV(DXGI_FORMAT_R32_FLOAT));


  m_FormatLookupTable.SetFormatInfo(
    ezGALResourceFormat::RGBAHalf,
    ezGALFormatLookupEntryDX11(DXGI_FORMAT_R16G16B16A16_TYPELESS).RT(DXGI_FORMAT_R16G16B16A16_FLOAT).VA(DXGI_FORMAT_R16G16B16A16_FLOAT).RV(DXGI_FORMAT_R16G16B16A16_FLOAT));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalizedsRGB, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB).RV(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB));
  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::RGBAUByteNormalized, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R8G8B8A8_TYPELESS).RT(DXGI_FORMAT_R8G8B8A8_UNORM).RV(DXGI_FORMAT_R8G8B8A8_UNORM));

  m_FormatLookupTable.SetFormatInfo(ezGALResourceFormat::D24S8, ezGALFormatLookupEntryDX11(DXGI_FORMAT_R24G8_TYPELESS).DS(DXGI_FORMAT_D24_UNORM_S8_UINT).D(DXGI_FORMAT_R24_UNORM_X8_TYPELESS).S(DXGI_FORMAT_X24_TYPELESS_G8_UINT));

}