using System.IO;
using UnrealBuildTool;

public class MySVE : ModuleRules
{
    public MySVE(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "Renderer",
            "RenderCore",
            "RHI",
            "Projects"
        });

        PrivateIncludePathModuleNames.AddRange(new string[]
        {
            "Renderer"
        });

        // dostep do Internal headerow Renderera - ok
        string EnginePath = Path.GetFullPath(Target.RelativeEnginePath);
        PrivateIncludePaths.Add(Path.Combine(EnginePath, "Source/Runtime/Renderer/Internal"));
        PrivateIncludePaths.Add(Path.Combine(EnginePath, "Source/Runtime/Renderer/Internal/PostProcess"));
    }
}