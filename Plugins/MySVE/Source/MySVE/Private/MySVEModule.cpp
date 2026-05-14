#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "MySVEExtension.h"
#include "Misc/CoreDelegates.h"

class FMySVEModule : public IModuleInterface
{
public:
    TSharedPtr<FMySVEExtension, ESPMode::ThreadSafe> MySVEExtension;

    virtual void StartupModule() override
    {
        FString PluginShaderDir = FPaths::Combine(
            IPluginManager::Get().FindPlugin(TEXT("MySVE"))->GetBaseDir(),
            TEXT("Shaders")
        );

        AddShaderSourceDirectoryMapping(TEXT("/Plugin/MySVE"), PluginShaderDir);

        FCoreDelegates::OnPostEngineInit.AddRaw(
            this,
            &FMySVEModule::OnPostEngineInit
        );

        UE_LOG(LogTemp, Warning, TEXT("MySVE: Module started!"));
    }

    void OnPostEngineInit()
    {
        UE_LOG(LogTemp, Warning, TEXT("MySVE: Creating extension"));

        MySVEExtension =
            FSceneViewExtensions::NewExtension<FMySVEExtension>();
    }

    virtual void ShutdownModule() override
    {
        MySVEExtension.Reset();
    }
};

IMPLEMENT_MODULE(FMySVEModule, MySVE)