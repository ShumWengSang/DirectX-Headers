// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif
#include "gtest/gtest.h"
#include <windows.h>
#include <dxgi1_6.h>
#include <intsafe.h>
#include "directx/d3dx12.h"

inline std::string HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
    HrException(HRESULT hr) : std::runtime_error(HrToString(hr)), m_hr(hr) {}
    HRESULT Error() const { return m_hr; }
private:
    const HRESULT m_hr;
};

inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw HrException(hr);
    }
}

/*******************************************************************/
/*   This test compared with the GetCopytableFootprint in device   */
/*******************************************************************/
using Microsoft::WRL::ComPtr;
class Direct3DInstance : public ::testing::Test
{
protected:
    ComPtr<ID3D12Device> m_device;

protected:
    void SetUp() override {

        // Create factory for hardware adapter
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

        // Get hardware adapter
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter, false);

        // Create device
        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&m_device)
        ));


    }


    void TearDown() override {

    }

    void GetHardwareAdapter(
        IDXGIFactory1* pFactory,
        IDXGIAdapter1** ppAdapter,
        bool requestHighPerformanceAdapter)
    {
        *ppAdapter = nullptr;

        ComPtr<IDXGIAdapter1> adapter;

        ComPtr<IDXGIFactory6> factory6;
        if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
        {
            for (
                UINT adapterIndex = 0;
                SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                    adapterIndex,
                    requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                    IID_PPV_ARGS(&adapter)));
                ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        if (adapter.Get() == nullptr)
        {
            for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
                {
                    // Don't select the Basic Render Driver adapter.
                    // If you want a software adapter, pass in "/warp" on the command line.
                    continue;
                }

                // Check to see whether the adapter supports Direct3D 12, but don't create the
                // actual device yet.
                if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
                {
                    break;
                }
            }
        }

        *ppAdapter = adapter.Detach();
    }
};


// Test for no subresource
TEST_F(Direct3DInstance, NoSubresourceGetFootprint) {

    // Create a resource descriptor for a texture
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = 100;
    textureDesc.Height = 100;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Use the internal and 100% correct ID3D12Device::GetCopyableFootprint to compare against
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT correctFootprint;
    UINT correctNumberOfRows;
    UINT64 correctRowSizeInBytes;
    UINT64 correctTotalBytes;

    m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &correctFootprint, &correctNumberOfRows, &correctRowSizeInBytes, &correctTotalBytes);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT testFootprint;
    UINT testNumberOfRows;
    UINT64 testRowSizeInBytes;
    UINT64 testTotalBytes;
    D3DX12GetCopyableFootprints(textureDesc, 0, 1, 0, &testFootprint, &testNumberOfRows, &testRowSizeInBytes, &testTotalBytes);

    EXPECT_TRUE(0 == std::memcmp(&correctFootprint, &testFootprint, sizeof(correctFootprint)));
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctRowSizeInBytes, testRowSizeInBytes);
    EXPECT_EQ(correctTotalBytes, testTotalBytes);
}

// Test for mipmap subresource
TEST_F(Direct3DInstance, MimapSubresourceGetFootprint) {

    // Create a resource descriptor for a texture
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 0;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = 100;
    textureDesc.Height = 100;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Use the internal and 100% correct ID3D12Device::GetCopyableFootprint to compare against
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT correctFootprint;
    UINT correctNumberOfRows;
    UINT64 correctRowSizeInBytes;
    UINT64 correctTotalBytes;

    m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &correctFootprint, &correctNumberOfRows, &correctRowSizeInBytes, &correctTotalBytes);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT testFootprint;
    UINT testNumberOfRows;
    UINT64 testRowSizeInBytes;
    UINT64 testTotalBytes;
    D3DX12GetCopyableFootprints(textureDesc, 0, 1, 0, &testFootprint, &testNumberOfRows, &testRowSizeInBytes, &testTotalBytes);

    EXPECT_TRUE(0 == std::memcmp(&correctFootprint, &testFootprint, sizeof(correctFootprint)));
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctRowSizeInBytes, testRowSizeInBytes);
    EXPECT_EQ(correctTotalBytes, testTotalBytes);
}

// Test for array slices subresource
TEST_F(Direct3DInstance, ArraySlicesSubresourceGetFootprint) {

    // Create a resource descriptor for a texture
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = 100;
    textureDesc.Height = 100;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 5;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Use the internal and 100% correct ID3D12Device::GetCopyableFootprint to compare against
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT correctFootprint;
    UINT correctNumberOfRows;
    UINT64 correctRowSizeInBytes;
    UINT64 correctTotalBytes;

    m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &correctFootprint, &correctNumberOfRows, &correctRowSizeInBytes, &correctTotalBytes);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT testFootprint;
    UINT testNumberOfRows;
    UINT64 testRowSizeInBytes;
    UINT64 testTotalBytes;
    D3DX12GetCopyableFootprints(textureDesc, 0, 1, 0, &testFootprint, &testNumberOfRows, &testRowSizeInBytes, &testTotalBytes);

    EXPECT_TRUE(0 == std::memcmp(&correctFootprint, &testFootprint, sizeof(correctFootprint)));
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctRowSizeInBytes, testRowSizeInBytes);
    EXPECT_EQ(correctTotalBytes, testTotalBytes);
}

// Test for planes subresource
TEST_F(Direct3DInstance, PlanesSubresourceGetFootprint) {

    // Create a resource descriptor for a texture
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_NV12;
    textureDesc.Width = 100;
    textureDesc.Height = 100;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Use the internal and 100% correct ID3D12Device::GetCopyableFootprint to compare against
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT correctFootprint;
    UINT correctNumberOfRows;
    UINT64 correctRowSizeInBytes;
    UINT64 correctTotalBytes;

    m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &correctFootprint, &correctNumberOfRows, &correctRowSizeInBytes, &correctTotalBytes);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT testFootprint;
    UINT testNumberOfRows;
    UINT64 testRowSizeInBytes;
    UINT64 testTotalBytes;
    D3DX12GetCopyableFootprints(textureDesc, 0, 1, 0, &testFootprint, &testNumberOfRows, &testRowSizeInBytes, &testTotalBytes);

    EXPECT_TRUE(0 == std::memcmp(&correctFootprint, &testFootprint, sizeof(correctFootprint)));
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctRowSizeInBytes, testRowSizeInBytes);
    EXPECT_EQ(correctTotalBytes, testTotalBytes);
}

// Comprehensive test for subresource
TEST_F(Direct3DInstance, ComprehensiveSubresourceGetFootprint) {

    // Create a resource descriptor for a texture
    D3D12_RESOURCE_DESC textureDesc = {};
    textureDesc.MipLevels = 0;
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    textureDesc.Width = 100;
    textureDesc.Height = 100;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    textureDesc.DepthOrArraySize = 5;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    // Use the internal and 100% correct ID3D12Device::GetCopyableFootprint to compare against
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT correctFootprint;
    UINT correctNumberOfRows;
    UINT64 correctRowSizeInBytes;
    UINT64 correctTotalBytes;

    m_device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &correctFootprint, &correctNumberOfRows, &correctRowSizeInBytes, &correctTotalBytes);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT testFootprint;
    UINT testNumberOfRows;
    UINT64 testRowSizeInBytes;
    UINT64 testTotalBytes;
    D3DX12GetCopyableFootprints(textureDesc, 0, 1, 0, &testFootprint, &testNumberOfRows, &testRowSizeInBytes, &testTotalBytes);

    EXPECT_TRUE(0 == std::memcmp(&correctFootprint, &testFootprint, sizeof(correctFootprint)));
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctNumberOfRows, testNumberOfRows);
    EXPECT_EQ(correctRowSizeInBytes, testRowSizeInBytes);
    EXPECT_EQ(correctTotalBytes, testTotalBytes);
}