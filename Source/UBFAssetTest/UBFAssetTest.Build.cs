using UnrealBuildTool;

public class UBFAssetTest : ModuleRules
{
    public UBFAssetTest(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core", "UBF", "UBFAPIController", "FutureverseUBFController", "HTTP"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UMG",
                "Json",
                "JsonUtilities",
            }
        );
    }
}