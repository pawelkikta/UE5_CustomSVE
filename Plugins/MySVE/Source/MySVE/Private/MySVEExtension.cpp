#include "MySVEExtension.h"
#include "RenderGraph.h"
#include "RenderGraphUtils.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "PostProcessInputs.h"
#include "ScreenPass.h"
#include "SceneViewExtension.h"

static TAutoConsoleVariable<int32> CVarMySVEEnabled(
    TEXT("MySVE.Enable"),
    1,
    TEXT("0=off, 1=on"),
    ECVF_RenderThreadSafe
);

class FRedOverlayCS : public FGlobalShader
{
    DECLARE_SHADER_TYPE(FRedOverlayCS, Global);
    SHADER_USE_PARAMETER_STRUCT(FRedOverlayCS, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, SceneColorOutput)
        SHADER_PARAMETER(FIntPoint, ViewSize)
    END_SHADER_PARAMETER_STRUCT()

public:
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};

IMPLEMENT_SHADER_TYPE(, FRedOverlayCS, TEXT("/Plugin/MySVE/RedOverlay.usf"), TEXT("MainCS"), SF_Compute);

FMySVEExtension::FMySVEExtension(const FAutoRegister& AutoRegister)
    : FSceneViewExtensionBase(AutoRegister)
{
}

void FMySVEExtension::PrePostProcessPass_RenderThread(
    FRDGBuilder& GraphBuilder,
    const FSceneView& View,
    const FPostProcessingInputs& Inputs)
{
    if (CVarMySVEEnabled.GetValueOnRenderThread() == 0)
    {
        return;
    }

    // SceneColor z Inputs
    FRDGTextureRef SceneColorTexture = (*Inputs.SceneTextures)->SceneColorTexture;

    if (!SceneColorTexture)
    {
        return;
    }

    FIntPoint ViewSize = View.UnscaledViewRect.Size();

    // UAV z SceneColor
    FRDGTextureUAVRef SceneColorUAV = GraphBuilder.CreateUAV(SceneColorTexture);

    // parametry shadera
    auto PassParams = GraphBuilder.AllocParameters<FRedOverlayCS::FParameters>();
    PassParams->SceneColorOutput = SceneColorUAV;
    PassParams->ViewSize = ViewSize;

    TShaderMapRef<FRedOverlayCS> ComputeShader(GetGlobalShaderMap(View.GetFeatureLevel()));

    FIntVector GroupCount = FComputeShaderUtils::GetGroupCount(ViewSize, FIntPoint(8, 8));

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("MySVE Red Overlay"),
        ComputeShader,
        PassParams,
        GroupCount
    );
}