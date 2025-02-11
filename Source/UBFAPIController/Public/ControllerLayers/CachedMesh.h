#pragma once
#include "glTFRuntimeParser.h"


class UglTFRuntimeAsset;

struct FCachedMeshEntry
{
	FCachedMeshEntry() {}
	FCachedMeshEntry(const FglTFRuntimeConfig& ImportConfig, UglTFRuntimeAsset* Asset) {}
	
	FglTFRuntimeConfig ImportConfig;
	TWeakObjectPtr<UglTFRuntimeAsset> Asset;

	// Custom comparison function for FglTFRuntimeBasisMatrix
	static bool AreBasisMatricesEqual(const FglTFRuntimeBasisMatrix& A, const FglTFRuntimeBasisMatrix& B)
	{
		return A.XAxis.Equals(B.XAxis, KINDA_SMALL_NUMBER) &&
			   A.YAxis.Equals(B.YAxis, KINDA_SMALL_NUMBER) &&
			   A.ZAxis.Equals(B.ZAxis, KINDA_SMALL_NUMBER) &&
			   A.Origin.Equals(B.Origin, KINDA_SMALL_NUMBER);
	}

	// Custom comparison function for FglTFRuntimeConfig
	static bool AreImportConfigsEqual(const FglTFRuntimeConfig& A, const FglTFRuntimeConfig& B)
	{
		return A.TransformBaseType == B.TransformBaseType &&
			   A.BasisMatrix.Equals(B.BasisMatrix, KINDA_SMALL_NUMBER) &&
			   A.BaseTransform.Equals(B.BaseTransform, KINDA_SMALL_NUMBER) &&
			   AreBasisMatricesEqual(A.BasisVectorMatrix, B.BasisVectorMatrix) &&
			   FMath::IsNearlyEqual(A.SceneScale, B.SceneScale, KINDA_SMALL_NUMBER) &&
			   A.ContentPluginsToScan == B.ContentPluginsToScan &&
			   A.bAllowExternalFiles == B.bAllowExternalFiles &&
			   A.OverrideBaseDirectory == B.OverrideBaseDirectory &&
			   A.bOverrideBaseDirectoryFromContentDir == B.bOverrideBaseDirectoryFromContentDir &&
			   A.ArchiveEntryPoint == B.ArchiveEntryPoint &&
			   A.ArchiveAutoEntryPointExtensions == B.ArchiveAutoEntryPointExtensions &&
			   A.bSearchContentDir == B.bSearchContentDir &&
			   A.RuntimeContextObject == B.RuntimeContextObject &&  // Pointer comparison
			   A.RuntimeContextString == B.RuntimeContextString &&
			   A.bAsBlob == B.bAsBlob &&
			   A.PrefixForUnnamedNodes == B.PrefixForUnnamedNodes &&
			   A.EncryptionKey == B.EncryptionKey;
	}


	// Equality operator comparing only ImportConfig
	bool operator==(const FCachedMeshEntry& Other) const
	{
		return AreImportConfigsEqual(ImportConfig, Other.ImportConfig);
	}

	bool operator!=(const FCachedMeshEntry& Other) const
	{
		return !(*this == Other);
	}
};

struct FCachedMesh
{
	FCachedMesh() {}

	bool ContainsMesh(const FglTFRuntimeConfig& Config);
	void AddOrReplaceMesh(const FglTFRuntimeConfig& Config, UglTFRuntimeAsset* Asset);
	UglTFRuntimeAsset* GetMesh(const FglTFRuntimeConfig& Config);

private:
	int GetIndexForConfig(const FglTFRuntimeConfig& Config);

	
	TArray<FCachedMeshEntry> LoadedAssets;
};