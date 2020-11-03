#include "Common_3/OS/Interfaces/IApp.h"
#include "Common_3/OS/Interfaces/IFileSystem.h"
#include "Common_3/Renderer/IRenderer.h"
#include "Common_3/Renderer/IResourceLoader.h"

#include <iostream>

const uint32_t gImageCount = 3;

Renderer* pRenderer = nullptr;

Queue*   pGraphicsQueue = nullptr;
CmdPool* pCmdPools[gImageCount] = { nullptr };
Cmd*     pCmds[gImageCount] = { nullptr };

SwapChain*    pSwapChain = nullptr;
RenderTarget* pDepthBuffer = nullptr;
Fence*        pRenderCompleteFences[gImageCount] = { nullptr };
Semaphore*    pImageAcquiredSemaphore = nullptr;
Semaphore*    pRenderCompleteSemaphores[gImageCount] = { nullptr };

Shader*   pTriangleShader = nullptr;
Buffer*   pTriangleVertexBuffer = nullptr;
Pipeline* pTrianglePipeline = nullptr;

RootSignature* pRootSignature = nullptr;

uint32_t gFrameIndex = 0;

class MyApplication : public IApp
{
public:
    bool Init() override
    {
        // FILE PATHS
#if VULKAN
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders/vk");
#elif DIRECT3D12
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_SHADER_SOURCES, "Shaders/dx");
#endif
        fsSetPathForResourceDir(pSystemFileIO, RM_DEBUG,   RD_SHADER_BINARIES, "CompiledShaders");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_GPU_CONFIG, "GPUCfg");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_TEXTURES, "Textures");
        fsSetPathForResourceDir(pSystemFileIO, RM_CONTENT, RD_FONTS, "Fonts");

        // Window and Renderer setup
        RendererDesc settings = { nullptr };
        initRenderer(GetName(), &settings, &pRenderer);

        if (!pRenderer)
            return false;

        QueueDesc queueDesc = {};
        queueDesc.mType = QUEUE_TYPE_GRAPHICS;
        queueDesc.mFlag = QUEUE_FLAG_INIT_MICROPROFILE;
        addQueue(pRenderer, &queueDesc, &pGraphicsQueue);

        initResourceLoaderInterface(pRenderer);

        for (uint32_t i = 0; i < gImageCount; i++)
        {
            CmdPoolDesc cmdPoolDesc = {};
            cmdPoolDesc.pQueue = pGraphicsQueue;
            addCmdPool(pRenderer, &cmdPoolDesc, &pCmdPools[i]);

            CmdDesc cmdDesc = {};
            cmdDesc.pPool = pCmdPools[i];
            addCmd(pRenderer, &cmdDesc, &pCmds[i]);

            addFence(pRenderer, &pRenderCompleteFences[i]);
            addSemaphore(pRenderer, &pRenderCompleteSemaphores[i]);
        }
        addSemaphore(pRenderer, &pImageAcquiredSemaphore);

        ShaderLoadDesc triangleShaderLoadDesc = {};
        triangleShaderLoadDesc.mStages[0] = { "triangle.vert", nullptr, 0 };
        triangleShaderLoadDesc.mStages[1] = { "triangle.frag", nullptr, 0 };
        addShader(pRenderer, &triangleShaderLoadDesc, &pTriangleShader);

        Shader* shaders[] = { pTriangleShader };
        RootSignatureDesc rootSignatureDesc = {};
        rootSignatureDesc.mStaticSamplerCount = 0;
        rootSignatureDesc.mShaderCount = 1;
        rootSignatureDesc.ppShaders = shaders;
        addRootSignature(pRenderer, &rootSignatureDesc, &pRootSignature);

        // Generate vertex buffer
        float pPoints[] = {
                // Positions        // Colors
                 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // Bottom Right
                -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // Bottom Left
                 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f   // Top
        };

        uint64_t pointDataSize = 3 * 6 * sizeof(float);
        BufferLoadDesc pointVbDesc = {};
        pointVbDesc.mDesc.mDescriptors = DESCRIPTOR_TYPE_VERTEX_BUFFER;
        pointVbDesc.mDesc.mMemoryUsage = RESOURCE_MEMORY_USAGE_CPU_ONLY;
        pointVbDesc.mDesc.mSize = pointDataSize;
        pointVbDesc.pData = pPoints;
        pointVbDesc.ppBuffer = &pTriangleVertexBuffer;
        addResource(&pointVbDesc, nullptr);

        return true;
    }
    void Exit() override
    {
        waitQueueIdle(pGraphicsQueue);

        removeResource(pTriangleVertexBuffer);

        removeRootSignature(pRenderer, pRootSignature);
        removeShader(pRenderer, pTriangleShader);

        for (uint32_t i = 0; i < gImageCount; ++i)
        {
            removeFence(pRenderer, pRenderCompleteFences[i]);
            removeSemaphore(pRenderer, pRenderCompleteSemaphores[i]);

            removeCmd(pRenderer, pCmds[i]);
            removeCmdPool(pRenderer, pCmdPools[i]);
        }
        removeSemaphore(pRenderer, pImageAcquiredSemaphore);

        exitResourceLoaderInterface(pRenderer);
        removeQueue(pRenderer, pGraphicsQueue);
        removeRenderer(pRenderer);
    }

    bool Load() override
    {
        if (!addSwapChain())
            return false;

        // layout and pipeline for triangle draw
        VertexLayout vertexLayout = {};
        vertexLayout.mAttribCount = 2;
        vertexLayout.mAttribs[0].mSemantic = SEMANTIC_POSITION;
        vertexLayout.mAttribs[0].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[0].mBinding = 0;
        vertexLayout.mAttribs[0].mLocation = 0;
        vertexLayout.mAttribs[0].mOffset = 0;
        vertexLayout.mAttribs[1].mSemantic = SEMANTIC_COLOR;
        vertexLayout.mAttribs[1].mFormat = TinyImageFormat_R32G32B32_SFLOAT;
        vertexLayout.mAttribs[1].mBinding = 0;
        vertexLayout.mAttribs[1].mLocation = 1;
        vertexLayout.mAttribs[1].mOffset = 3 * sizeof(float);

        RasterizerStateDesc rasterizerStateDesc = {};
        rasterizerStateDesc.mCullMode = CULL_MODE_NONE;

        PipelineDesc pipelineDesc = {};
        pipelineDesc.mType = PIPELINE_TYPE_GRAPHICS;
        GraphicsPipelineDesc& graphicsPipelineDesc = pipelineDesc.mGraphicsDesc;
        graphicsPipelineDesc.mPrimitiveTopo = PRIMITIVE_TOPO_TRI_LIST;
        graphicsPipelineDesc.mRenderTargetCount = 1;
        graphicsPipelineDesc.pColorFormats = &pSwapChain->ppRenderTargets[0]->mFormat;
        graphicsPipelineDesc.mSampleCount = pSwapChain->ppRenderTargets[0]->mSampleCount;
        graphicsPipelineDesc.mSampleQuality = pSwapChain->ppRenderTargets[0]->mSampleQuality;
        graphicsPipelineDesc.pRootSignature = pRootSignature;
        graphicsPipelineDesc.pShaderProgram = pTriangleShader;
        graphicsPipelineDesc.pVertexLayout = &vertexLayout;
        graphicsPipelineDesc.pRasterizerState = &rasterizerStateDesc;
        addPipeline(pRenderer, &pipelineDesc, &pTrianglePipeline);

        return true;
    }
    void Unload() override
    {
        waitQueueIdle(pGraphicsQueue);

        removePipeline(pRenderer, pTrianglePipeline);

        removeSwapChain(pRenderer, pSwapChain);
    }

    void Update(float deltaTime) override {}
    void Draw() override
    {
        uint32_t swapchainImageIndex;
        acquireNextImage(pRenderer, pSwapChain, pImageAcquiredSemaphore, nullptr, &swapchainImageIndex);

        RenderTarget* renderTarget = pSwapChain->ppRenderTargets[swapchainImageIndex];
        Semaphore* renderCompleteSemaphore = pRenderCompleteSemaphores[gFrameIndex];
        Fence* renderCompleteFence = pRenderCompleteFences[gFrameIndex];

        // Stall if CPU is running "Swap Chain Buffer Count" frames ahead of GPU
        FenceStatus fenceStatus;
        getFenceStatus(pRenderer, renderCompleteFence, &fenceStatus);
        if (fenceStatus == FENCE_STATUS_INCOMPLETE)
            waitForFences(pRenderer, 1, &renderCompleteFence);

        // Reset cmd pool for this frame
        resetCmdPool(pRenderer, pCmdPools[gFrameIndex]);

        Cmd* cmd = pCmds[gFrameIndex];
        beginCmd(cmd);

        RenderTargetBarrier barriers[] = {
                { renderTarget, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET },
        };
        cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);

        // simply record the screen cleaning command
        LoadActionsDesc loadActionsDesc = {};
        loadActionsDesc.mLoadActionsColor[0] = LOAD_ACTION_CLEAR;
        cmdBindRenderTargets(cmd, 1, &renderTarget, nullptr, &loadActionsDesc, nullptr, nullptr, -1, -1);
        cmdSetViewport(cmd, 0.0f, 0.0f, (float)renderTarget->mWidth, (float)renderTarget->mHeight, 0.0f, 1.0f);
        cmdSetScissor(cmd, 0, 0, renderTarget->mWidth, renderTarget->mHeight);

        // draw triangle
        const uint32_t triangleVbStride = sizeof(float) * 6;
        cmdBindPipeline(cmd, pTrianglePipeline);
        cmdBindVertexBuffer(cmd, 1, &pTriangleVertexBuffer, &triangleVbStride, nullptr);
        cmdDraw(cmd, 3, 0);


        cmdBindRenderTargets(cmd, 0, NULL, NULL, NULL, NULL, NULL, -1, -1);

        barriers[0] = { renderTarget, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT };
        cmdResourceBarrier(cmd, 0, nullptr, 0, nullptr, 1, barriers);

        endCmd(cmd);


        QueueSubmitDesc submitDesc = {};
        submitDesc.mCmdCount = 1;
        submitDesc.mSignalSemaphoreCount = 1;
        submitDesc.mWaitSemaphoreCount = 1;
        submitDesc.ppCmds = &cmd;
        submitDesc.ppSignalSemaphores = &renderCompleteSemaphore;
        submitDesc.ppWaitSemaphores = &pImageAcquiredSemaphore;
        submitDesc.pSignalFence = renderCompleteFence;
        queueSubmit(pGraphicsQueue, &submitDesc);

        QueuePresentDesc presentDesc = {};
        presentDesc.mIndex = swapchainImageIndex;
        presentDesc.mWaitSemaphoreCount = 1;
        presentDesc.pSwapChain = pSwapChain;
        presentDesc.ppWaitSemaphores = &renderCompleteSemaphore;
        presentDesc.mSubmitDone = true;
        queuePresent(pGraphicsQueue, &presentDesc);

        gFrameIndex = (gFrameIndex + 1) % gImageCount;
    }

    const char* GetName() override { return "Hello The-Forge!"; }

    bool addSwapChain()
    {
        SwapChainDesc swapChainDesc = {};
        swapChainDesc.mWindowHandle = pWindow->handle;
        swapChainDesc.mPresentQueueCount = 1;
        swapChainDesc.ppPresentQueues = &pGraphicsQueue;
        swapChainDesc.mWidth = mSettings.mWidth;
        swapChainDesc.mHeight = mSettings.mHeight;
        swapChainDesc.mImageCount = gImageCount;
        swapChainDesc.mColorFormat = getRecommendedSwapchainFormat(true);
        swapChainDesc.mEnableVsync = mSettings.mDefaultVSyncEnabled;
        ::addSwapChain(pRenderer, &swapChainDesc, &pSwapChain);

        return pSwapChain != nullptr;
    }
};

//DEFINE_APPLICATION_MAIN(MyApplication);

extern int WindowsMain(int argc, char** argv, IApp* app);

int main(int argc, char** argv)
{
    MyApplication app;
    return WindowsMain(argc, argv, &app);
}
