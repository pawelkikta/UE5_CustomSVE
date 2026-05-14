#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "MySVEExtension.h"

class FMySVEModule : public IModuleInterface
{
public:
    TSharedPtr<FMySVEExtension> MySVEExtension;

    virtual void StartupModule() override
    {
        // Zarejestruj folder shaderow
        FString PluginShaderDir = FPaths::Combine(
            IPluginManager::Get().FindPlugin(TEXT("MySVE"))->GetBaseDir(),
            TEXT("Shaders")
        );
        AddShaderSourceDirectoryMapping(TEXT("/Plugin/MySVE"), PluginShaderDir);

        MySVEExtension = FSceneViewExtensions::NewExtension<FMySVEExtension>();
        UE_LOG(LogTemp, Warning, TEXT("MySVE: Module started!"));
    }

    virtual void ShutdownModule() override
    {
        MySVEExtension.Reset();
    }
};

IMPLEMENT_MODULE(FMySVEModule, MySVE)