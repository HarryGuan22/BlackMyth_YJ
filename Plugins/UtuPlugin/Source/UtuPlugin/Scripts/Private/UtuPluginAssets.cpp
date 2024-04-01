// Copyright Alex Quevillon. All Rights Reserved.

#include "UtuPlugin/Scripts/Public/UtuPluginAssets.h"
#include "UtuPlugin/Scripts/Public/UtuPluginPaths.h"
#include "UtuPlugin/Scripts/Public/UtuPluginConstants.h"
#include "UtuPlugin/Scripts/Public/UtuPluginLog.h"
#include "UtuPlugin/Scripts/Public/UtuPluginLibrary.h"

#include "Developer/AssetTools/Public/IAssetTools.h"
#include "Developer/AssetTools/Public/AssetToolsModule.h"
#include "Editor/UnrealEd/Public/AssetImportTask.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Editor/UnrealEd/Classes/Factories/MaterialFactoryNew.h"
#include "Runtime/Engine/Classes/Materials/Material.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"
#include "Runtime/Engine/Public/ComponentReregisterContext.h"
#include "Editor/UnrealEd/Classes/Factories/FbxImportUI.h"
#include "Editor/UnrealEd/Classes/Factories/FbxStaticMeshImportData.h"
#include "Editor/UnrealEd/Classes/Factories/FbxSkeletalMeshImportData.h"
//#include "Edutor/UnrealEd/Classes/Factories/FbxAnimSequenceImportData.h" /// TODO : Support animations
#include "Runtime/Engine/Classes/Engine/StaticMesh.h"
#include "Runtime/Engine/Classes/Engine/SkeletalMesh.h"
#include "Editor/UnrealEd/Public/Kismet2/KismetEditorUtilities.h"
#include "Editor/KismetCompiler/Public/KismetCompilerModule.h"
#include "UnrealEd/Public/Kismet2/CompilerResultsLog.h"
#include "Runtime/Engine/Classes/Engine/SimpleConstructionScript.h"
#include "Runtime/Engine/Classes/Engine/SCS_Node.h"
#include "Runtime/Engine/Classes/Animation/AnimSequence.h"
//#include "Editor/UnrealEd/Public/Toolkits/AssetEditorManager.h"
//#include "Editor/Kismet/Public/BlueprintEditor.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor/UnrealEd/Classes/Factories/WorldFactory.h"
#include "Editor/UnrealEd/Classes/ActorFactories/ActorFactoryEmptyActor.h"
#include "Runtime/Engine/Classes/Engine/StaticMeshActor.h"
#include "Runtime/Engine/Classes/Animation/SkeletalMeshActor.h"
#include "Runtime/AssetRegistry/Public/IAssetRegistry.h"
#include "BlueprintEditorSettings.h"
//#include "Editor/BlueprintGraph/Public/BlueprintEditorImportSettings.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Runtime/Engine/Classes/Kismet/GameplayStatics.h"
#include "Runtime/Engine/Classes/Materials/MaterialExpressionComponentMask.h"
#include "Runtime/Engine/Classes/Engine/DirectionalLight.h"
#include "Runtime/Engine/Classes/Engine/PointLight.h"
#include "Runtime/Engine/Classes/Engine/SpotLight.h"
#include "Runtime/Engine/Classes/Engine/SkyLight.h"
#include "Runtime/Engine/Classes/Components/LightComponent.h"
#include "Runtime/Engine/Classes/Components/PointLightComponent.h"
#include "Runtime/Engine/Classes/Components/SpotLightComponent.h"
#include "Runtime/Engine/Classes/Components/DirectionalLightComponent.h"
#include "Runtime/Engine/Classes/Components/SkyLightComponent.h"
#include "Runtime/Engine/Classes/Components/SkeletalMeshComponent.h"
#include "Runtime/CinematicCamera/Public/CineCameraActor.h"
#include "Runtime/CinematicCamera/Public/CineCameraComponent.h"
#include "Runtime/Engine/Classes/Camera/CameraActor.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "UnrealEd/Public/Kismet2/ComponentEditorUtils.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Engine/TextureCube.h"
#include "MaterialEditingLibrary.h"

#include "Runtime/Launch/Resources/Version.h" 

void FUtuPluginAssetTypeProcessor::Import(FUtuPluginJson Json, EUtuAssetType AssetType, bool executeFullImportOnSameFrame, bool DeleteInvalidAssets) {
	BeginImport(Json, AssetType, DeleteInvalidAssets);
	if (executeFullImportOnSameFrame) {
		while (ContinueImport() != true) {
			// ContinueImport
		}
	}
}

void FUtuPluginAssetTypeProcessor::BeginImport(FUtuPluginJson Json, EUtuAssetType AssetType, bool DeleteInvalidAssets) {
	AssetTools = FModuleManager::LoadModulePtr<FAssetToolsModule>("AssetTools");
	json = Json;
	assetType = AssetType;
	bDeleteInvalidAssets = DeleteInvalidAssets;
	amountItemsToProcess = GetAssetsNum();
	countItemsToProcess = 1;
	percentItemsToProcess = (float)countItemsToProcess / (float)amountItemsToProcess;
	if (amountItemsToProcess == 0) {
		bIsValid = false;
	}
}

bool FUtuPluginAssetTypeProcessor::ContinueImport() {
	if (GetAssetsNum() == 0) {
		//UTU_LOG_E("FUtuPluginAssetTypeProcessor::ContinueImport() Was called even though the list is already empty. This should never happen!");
		CompleteImport();
		return true;
	}
	switch (assetType) {
	case EUtuAssetType::Scene:
		nameItemToProcess = json.scenes[0].asset_name;
		ProcessScene(json.scenes[0]);
		json.scenes.RemoveAt(0);
		break;
	case EUtuAssetType::Mesh:
		nameItemToProcess = json.meshes[0].asset_name;
		ProcessMesh(json.meshes[0]);
		json.meshes.RemoveAt(0);
		break;
	case EUtuAssetType::Material:
		nameItemToProcess = json.materials[0].asset_name;
		ProcessMaterial(json.materials[0]);
		json.materials.RemoveAt(0);
		break;
	case EUtuAssetType::Texture:
		nameItemToProcess = json.textures[0].asset_name;
		ProcessTexture(json.textures[0]);
		json.textures.RemoveAt(0);
		break;
	case EUtuAssetType::PrefabFirstPass:
		nameItemToProcess = json.prefabs_first_pass[0].asset_name;
		ProcessPrefabFirstPass(json.prefabs_first_pass[0]);
		json.prefabs_first_pass.RemoveAt(0);
		break;
	case EUtuAssetType::PrefabSecondPass:
		nameItemToProcess = json.prefabs_second_pass[0].asset_name;
		ProcessPrefabSecondPass(json.prefabs_second_pass[0]);
		json.prefabs_second_pass.RemoveAt(0);
		break;
	default:
		break;
	}
	countItemsToProcess++;
	percentItemsToProcess = (float)countItemsToProcess / (float)FMath::Max(amountItemsToProcess, 1);
	if (GetAssetsNum() == 0) {
		CompleteImport();
		return true;
	}
	return false;
}

void FUtuPluginAssetTypeProcessor::CompleteImport() {

}

TArray<FString> FUtuPluginAssetTypeProcessor::FormatRelativeFilenameForUnreal(FString InRelativeFilename, bool bInIsMaterial) {
	if (InRelativeFilename != "") {
		FString Relative = InRelativeFilename;
		if (Relative.RemoveFromStart("Assets")) {
			Relative = "/Game" + Relative;
		}
		if (bInIsMaterial) {
			// Fix the fact that materials doesn't really exists in Unity if they are from the .fbx file -.-
			Relative = Relative.Replace(TEXT(".fbx"), TEXT("_Mat"));
			// Same when it come from a .mat file --.--
			Relative = Relative.Replace(TEXT(".mat"), TEXT("_Mat"));
		}
		Relative = Relative.Replace(*UtuPluginPaths::backslash, *UtuPluginPaths::slash);
		Relative = Relative.Replace(TEXT(" "), TEXT("_"));
		if (Relative.Contains(".")) {
			Relative.Split(".", &Relative, nullptr, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		}
		if (!Relative.StartsWith("/Game")) {
			Relative = "/UtuPlugin/Default/" + Relative.Replace(TEXT("Resources"), TEXT(""));
			//Relative = "/Game/INVALID_FILENAME/" + Relative;
		}
		Relative = Relative.Replace(TEXT("."), TEXT("_")); // Dots in asset path? Really!? -.-
		FString Path;
		FString Filename;
		Relative.Split("/", &Path, &Filename, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		return { Path, Filename, Path / Filename };
	}
	return {"", "", ""};
}

int FUtuPluginAssetTypeProcessor::GetAssetsNum() {
	switch (assetType) {
	case EUtuAssetType::Scene:
		return json.scenes.Num();
		break;
	case EUtuAssetType::Mesh:
		return json.meshes.Num();
		break;
	case EUtuAssetType::Material:
		return json.materials.Num();
		break;
	case EUtuAssetType::Texture:
		return json.textures.Num();
		break;
	case EUtuAssetType::PrefabFirstPass:
		return json.prefabs_first_pass.Num();
		break;
	case EUtuAssetType::PrefabSecondPass:
		return json.prefabs_second_pass.Num();
		break;
	default:
		break;
	}
	return 0;
}

void FUtuPluginAssetTypeProcessor::ProcessScene(FUtuPluginScene InUtuScene) {
	// Format Paths
	TArray<FString> AssetNames = StartProcessAsset(InUtuScene);
	// Invalid Asset
	if (DeleteInvalidAssetIfNeeded(AssetNames, UWorld::StaticClass())) {
		// Existing Asset
		UWorld* Asset = Cast<UWorld>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
		LogAssetCreateOrNot(Asset);
		// Create Asset
		if (Asset == nullptr) {
			UPackage* Package = CreateAssetPackage(AssetNames[2], true);
			UWorldFactory* Factory = NewObject<UWorldFactory>();
			Asset = Cast<UWorld>(Factory->FactoryCreateNew(UWorld::StaticClass(), Package, FName(*AssetNames[1]), RF_Public | RF_Standalone, NULL, GWarn));
			LogAssetImportedOrFailed(Asset, AssetNames, "", "World", { });
		}
		// Process Asset
		if (Asset != nullptr) {
			Asset->PreEditChange(NULL);
			// Delete Old Actors -- TODO : Analyse them and keep the good ones
			TArray<AActor*> OldActors;
			UGameplayStatics::GetAllActorsWithTag(Asset, "UtuActor", OldActors);
			if (OldActors.Num() > 0) {
				UTU_LOG_L("        " + FString::FromInt(OldActors.Num()) + " old actors from previous import detected. Deleting them...");
				//UTU_LOG_L("            Plan for a future release: Analyse existing Actors and keep the good ones.");
				for (AActor* OldActor : OldActors) {
					Asset->DestroyActor(OldActor);
				}
			}
			// Start by creating a simple SkyLight
			WorldSpawnSkyLightActor(Asset);
			// Map
			TMap<int, AActor*> IdToActor;
			TMap<AActor*, int> ActorToParentId;
			// Spawn Actors
			for (FUtuPluginActor UtuActor : InUtuScene.scene_actors) {
				AActor* RootActor = WorldAddRootActorForSubActorsIfNeeded(Asset, UtuActor);
				if (RootActor != nullptr) {
					RootActor->Tags.Add("UtuActor");
					IdToActor.Add(UtuActor.actor_id, RootActor);
					ActorToParentId.Add(RootActor, UtuActor.actor_parent_id);
				}
				// Spawn Real Actors
				for (EUtuActorType ActorType : UtuActor.actor_types) {
					AActor* Actor = nullptr;
					if (ActorType == EUtuActorType::Empty) {
						// Don't care about an empty actor 'cause it's already spawned as a RootActor
					}
					else if (ActorType == EUtuActorType::StaticMesh) {
						Actor = WorldSpawnStaticMeshActor(Asset, UtuActor);
					}
					else if (ActorType == EUtuActorType::SkeletalMesh) {
						Actor = WorldSpawnSkeletalMeshActor(Asset, UtuActor); // TODO : Support Skeletal Mesh
						//Actor = WorldSpawnStaticMeshActor(Asset, UtuActor);
					}
					else if (ActorType == EUtuActorType::PointLight) {
						Actor = WorldSpawnPointLightActor(Asset, UtuActor);
					}
					else if (ActorType == EUtuActorType::DirectionalLight) {
						Actor = WorldSpawnDirectionalLightActor(Asset, UtuActor);
					}
					else if (ActorType == EUtuActorType::SpotLight) {
						Actor = WorldSpawnSpotLightActor(Asset, UtuActor);
					}
					else if (ActorType == EUtuActorType::Camera) {
						Actor = WorldSpawnCameraActor(Asset, UtuActor);
					}
					else if (ActorType == EUtuActorType::Prefab) {
						Actor = WorldSpawnBlueprintActor(Asset, UtuActor);
					}
					// Attachment
					if (Actor != nullptr) {
						Actor->GetRootComponent()->SetMobility(UtuActor.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
						if (RootActor == nullptr) {
							Actor->Tags.Add("UtuActor");
							Actor->SetActorLabel(UtuActor.actor_display_name);
							Actor->Tags.Add(*FString::FromInt(UtuActor.actor_id));
							if (UtuActor.actor_tag != "Untagged") {
								Actor->Tags.Add(*UtuActor.actor_tag);
							}
							Actor->SetActorHiddenInGame(!UtuActor.actor_is_visible);
							if (ActorType == EUtuActorType::StaticMesh && ImportSettings.bImportSeparated)
							{
								Actor->SetActorLocation(UtuConst::ConvertLocation(UtuActor.actor_world_location_if_separated));
							}
							else
							{
								Actor->SetActorLocation(UtuConst::ConvertLocation(UtuActor.actor_world_location));
							}
							Actor->SetActorRotation(UtuConst::ConvertRotation(UtuActor.actor_world_rotation));
							Actor->SetActorScale3D(UtuConst::ConvertScale(UtuActor.actor_world_scale));
							IdToActor.Add(UtuActor.actor_id, Actor);
							ActorToParentId.Add(Actor, UtuActor.actor_parent_id);
						}
						else {
							Actor->AttachToActor(RootActor, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, false));
						}
					}
				}
			}
			// Parent Actors
			UTU_LOG_L("        Parenting actors...");
			TArray<AActor*> Keys;
			ActorToParentId.GetKeys(Keys);
			for (AActor* Actor : Keys) {
				int Id = ActorToParentId[Actor];
				if (Id != UtuConst::INVALID_INT) {
					if (IdToActor.Contains(Id)) {
						AActor* ParentActor = IdToActor[Id];
						Actor->AttachToActor(ParentActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
						UTU_LOG_L("            " + Actor->GetActorLabel() + " -> " + ParentActor->GetActorLabel());
					}
					else {
						UTU_LOG_W("            Failed to find parent for: " + Actor->GetActorLabel());
						UTU_LOG_W("                Potential Causes:");
						UTU_LOG_W("                    - The desired parent of this actor is a component of a prefab.");
						UTU_LOG_W("                      (Scene edits of prefabs' components aren't supported yet.)");
						UTU_LOG_W("                    - The desired parent failed to spawn for some reason. (Missing Bp asset maybe?)");
					}
				}
			}
			Asset->MarkPackageDirty();
			Asset->PostEditChange();
		}
	}
}

void FUtuPluginAssetTypeProcessor::ProcessMesh(FUtuPluginMesh InUtuMesh) {
	if (InUtuMesh.asset_relative_filename.StartsWith("Assets")) { // Default Unity Mesh
		// Format Paths
		TArray<FString> AssetNames = StartProcessAsset(InUtuMesh);
		if (InUtuMesh.is_skeletal_mesh) {
			// Invalid Asset
			if (DeleteInvalidAssetIfNeeded(AssetNames, USkeletalMesh::StaticClass())) {
				// Existing Asset
				USkeletalMesh* Asset = Cast<USkeletalMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
				LogAssetImportOrReimport(Asset);
				// Create Asset
				UTU_LOG_L("        Fbx File Fullname: " + InUtuMesh.mesh_file_absolute_filename);
				UTU_LOG_L("        Import Scale Factor: " + FString::SanitizeFloat(InUtuMesh.mesh_import_scale_factor));
				UFbxImportUI* Options = NewObject<UFbxImportUI>();
				Options->bIsObjImport = InUtuMesh.mesh_file_absolute_filename.EndsWith(".obj", ESearchCase::IgnoreCase);
				Options->bAutomatedImportShouldDetectType = false;
				Options->bImportAnimations = false;
				Options->MeshTypeToImport = EFBXImportType::FBXIT_SkeletalMesh;
				Options->bImportMesh = true;
				Options->bImportTextures = false;
				Options->bImportMaterials = false;
				Options->bImportAsSkeletal = true;
				Options->SkeletalMeshImportData->NormalImportMethod = ImportSettings.NormalImportMethod;
				Options->SkeletalMeshImportData->NormalGenerationMethod = ImportSettings.NormalGenerationMethod;
				Options->SkeletalMeshImportData->bComputeWeightedNormals = ImportSettings.bComputeWeightedNormals;
				Options->SkeletalMeshImportData->bUseT0AsRefPose = ImportSettings.bUseT0AsRefPose;
				Options->SkeletalMeshImportData->bPreserveSmoothingGroups = ImportSettings.bPreserveSmoothingGroups;
				Options->SkeletalMeshImportData->bImportMeshesInBoneHierarchy = ImportSettings.bImportMeshesInBoneHierarchy;
				Options->SkeletalMeshImportData->bImportMorphTargets = ImportSettings.bImportMorphTargets;
				Options->SkeletalMeshImportData->bComputeWeightedNormals = ImportSettings.bComputeWeightedNormals;
				Options->SkeletalMeshImportData->bTransformVertexToAbsolute = true;
				Options->SkeletalMeshImportData->bConvertScene = true;
				Options->SkeletalMeshImportData->bForceFrontXAxis = true;
				Options->SkeletalMeshImportData->bConvertSceneUnit = true;
				Options->SkeletalMeshImportData->ImportTranslation = UtuConst::ConvertLocation(InUtuMesh.mesh_import_position_offset, true, InUtuMesh.mesh_import_rotation_offset) / FMath::Max(0.0001f, InUtuMesh.mesh_import_scale_offset.X);
				Options->SkeletalMeshImportData->ImportRotation = FRotator(UtuConst::ConvertRotation(InUtuMesh.mesh_import_rotation_offset, true));
				Options->SkeletalMeshImportData->ImportUniformScale = InUtuMesh.mesh_import_scale_factor / FMath::Max(0.0001f, InUtuMesh.mesh_import_scale_offset.X);
				AssetTools->Get().ImportAssetTasks({ BuildTask(InUtuMesh.mesh_file_absolute_filename, AssetNames, Options) });
				Asset = Cast<USkeletalMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
				LogAssetImportedOrFailed(Asset, AssetNames, InUtuMesh.mesh_file_absolute_filename, "SkeletalMesh", { "Invalid FBX : Make sure that the Fbx file is valid by trying to import it manually in Unreal." });
				// Process Asset
				if (Asset != nullptr) {
					Asset->PreEditChange(NULL);
					UTU_LOG_L("                Associating Materials to Skeletal Mesh...");
					Asset->Materials.Empty();
					for (int x = 0; x < InUtuMesh.mesh_materials_relative_filenames.Num(); x++) {
						TArray<FString> MatNames = FormatRelativeFilenameForUnreal(InUtuMesh.mesh_materials_relative_filenames[x], true);
						UTU_LOG_L("                    MaterialId[" + FString::FromInt(x) + "] : " + MatNames[2]);
						UMaterial* MaterialAsset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(MatNames[2]));
						Asset->Materials.Add(MaterialAsset);
						if (MaterialAsset == nullptr) {
							UTU_LOG_W("                        Failed to assign material because it doesn't exists: '" + MatNames[2] + "'");
						}
					}
					Asset->PostEditChange();
					// TODO : Support Animations
					//if (InUtuMesh.skeletal_mesh_animations_relative_filenames.Num() > 0) {
					//	UTU_LOG_L("                Importing Animations Associated with Skeletal Mesh...");
					//	for (FString x : InUtuMesh.skeletal_mesh_animations_relative_filenames) {
					//		TArray<FString> AnimNames = FormatRelativeFilenameForUnreal(x, false);
					//		// Invalid Asset
					//		if (DeleteInvalidAssetIfNeeded(AssetNames, UAnimSequence::StaticClass())) {
					//			// Existing Asset
					//			UAnimSequence* AnimAsset = Cast<UAnimSequence>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
					//			// Create Asset
					//			UTU_LOG_L("                    Fbx File Fullname: " + AnimNames[2]);
					//			UFbxImportUI* Options = NewObject<UFbxImportUI>();
					//			Options->bAutomatedImportShouldDetectType = false;
					//			Options->bImportAnimations = true;
					//			Options->MeshTypeToImport = EFBXImportType::FBXIT_Animation;
					//			Options->bImportMesh = false;
					//			Options->bImportTextures = false;
					//			Options->bImportMaterials = false;
					//			Options->bImportAsSkeletal = false;
					//			Options->AnimSequenceImportData->
					//			AssetTools->Get().ImportAssetTasks({ BuildTask(InUtuMesh.mesh_file_absolute_filename, AssetNames, Options) });
					//			AnimAsset = Cast<USkeletalMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
					//			LogAssetImportedOrFailed(AnimAsset, AssetNames, InUtuMesh.mesh_file_absolute_filename, "SkeletalMesh", { "Invalid FBX : Make sure that the Fbx file is valid by trying to import it manually in Unreal." });
					//		}
					//	}
					//}
				}
			}
		}
		else {
			// Invalid Asset
			if (DeleteInvalidAssetIfNeeded(AssetNames, UStaticMesh::StaticClass())) {
				// Existing Asset
				UStaticMesh* Asset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
				if (Asset == nullptr && ImportSettings.bImportSeparated) {
					// Try separated way
					Asset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2] + "_" + InUtuMesh.asset_name.Replace(TEXT(" "), TEXT("_")).Replace(TEXT(".fbx"), TEXT(""))));
				}
				LogAssetImportOrReimport(Asset);
				// Create Asset
				UTU_LOG_L("        Fbx File Fullname: " + InUtuMesh.mesh_file_absolute_filename);
				UTU_LOG_L("        Import Scale Factor: " + FString::SanitizeFloat(InUtuMesh.mesh_import_scale_factor));
				UFbxImportUI* Options = NewObject<UFbxImportUI>();
				Options->bIsObjImport = InUtuMesh.mesh_file_absolute_filename.EndsWith(".obj", ESearchCase::IgnoreCase);
				Options->bAutomatedImportShouldDetectType = false;
				Options->bImportAnimations = false;
				Options->MeshTypeToImport = EFBXImportType::FBXIT_StaticMesh;
				Options->bImportMesh = true;
				Options->bImportTextures = false;
				Options->bImportMaterials = false;
				Options->bImportAsSkeletal = false; 
				if (ImportSettings.bImportSeparated)
				{
					Options->StaticMeshImportData->bCombineMeshes = false;
					Options->StaticMeshImportData->bTransformVertexToAbsolute = true;
				}
				else
				{
					Options->StaticMeshImportData->bCombineMeshes = true;
					Options->StaticMeshImportData->bTransformVertexToAbsolute = false;
				}
				Options->StaticMeshImportData->bBakePivotInVertex = false;
				Options->StaticMeshImportData->bTransformVertexToAbsolute = true;
				Options->StaticMeshImportData->bConvertScene = true;
				Options->StaticMeshImportData->bForceFrontXAxis = true;
				Options->StaticMeshImportData->bConvertSceneUnit = true;
				Options->StaticMeshImportData->bAutoGenerateCollision = ImportSettings.bAutoGenerateCollision;
				Options->StaticMeshImportData->bGenerateLightmapUVs = ImportSettings.bGenerateLightmapUVs;
				Options->StaticMeshImportData->bRemoveDegenerates = ImportSettings.bRemoveDegenerates;
				Options->StaticMeshImportData->NormalImportMethod = ImportSettings.NormalImportMethod;
				Options->StaticMeshImportData->NormalGenerationMethod = ImportSettings.NormalGenerationMethod;
				Options->StaticMeshImportData->bComputeWeightedNormals = ImportSettings.bComputeWeightedNormals;
				Options->StaticMeshImportData->ImportTranslation = UtuConst::ConvertLocation(InUtuMesh.mesh_import_position_offset, true, InUtuMesh.mesh_import_rotation_offset) / FMath::Max(0.0001f, InUtuMesh.mesh_import_scale_offset.X);
				Options->StaticMeshImportData->ImportRotation = FRotator(UtuConst::ConvertRotation(InUtuMesh.mesh_import_rotation_offset, true));
				float Scale = InUtuMesh.mesh_import_scale_factor * (InUtuMesh.use_file_scale ? ImportSettings.MeshImportScaleMultiplierIfUseFileScale : ImportSettings.MeshImportScaleMultiplierIfNotUseFileScale);
				Options->StaticMeshImportData->ImportUniformScale = Scale / FMath::Max(0.0001f, InUtuMesh.mesh_import_scale_offset.X);
				//Options->StaticMeshImportData->ImportUniformScale = FMath::Max(0.0001f, InUtuMesh.mesh_import_scale_offset.X);
				//if (!InUtuMesh.mesh_import_convert_units)
				//{
				//	Options->StaticMeshImportData->ImportUniformScale = (InUtuMesh.mesh_import_scale_factor / Options->StaticMeshImportData->ImportUniformScale) * ImportSettings.ScaleMultiplierForNonConvertUnitMeshes;
				//}
				AssetTools->Get().ImportAssetTasks({ BuildTask(InUtuMesh.mesh_file_absolute_filename, AssetNames, Options) });
				Asset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
				if (Asset == nullptr && ImportSettings.bImportSeparated) {
					// Try separated way
					Asset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(AssetNames[2] + "_" + InUtuMesh.asset_name.Replace(TEXT(" "), TEXT("_")).Replace(TEXT(".fbx"), TEXT(""))));
				}
				LogAssetImportedOrFailed(Asset, AssetNames, InUtuMesh.mesh_file_absolute_filename, "StaticMesh", { "Invalid FBX : Make sure that the Fbx file is valid by trying to import it manually in Unreal." });
				// Process Asset
				if (Asset != nullptr) {
					UTU_LOG_L("                Associating Materials to Static Mesh...");
					for (int x = 0; x < InUtuMesh.mesh_materials_relative_filenames.Num(); x++) {
						TArray<FString> MatNames = FormatRelativeFilenameForUnreal(InUtuMesh.mesh_materials_relative_filenames[x], true);
						UTU_LOG_L("                    MaterialId[" + FString::FromInt(x) + "] : " + MatNames[2]);
						UMaterial* MaterialAsset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(MatNames[2]));
						Asset->SetMaterial(x, MaterialAsset);
						if (MaterialAsset == nullptr) {
							UTU_LOG_W("                        Failed to assign material because it doesn't exists: '" + MatNames[2] + "'");
						}
					}
				}
			}
		}
	}
}

UPackage* FUtuPluginAssetTypeProcessor::CreateAssetPackage(FString InRelativeFilename, bool bLoadPackage) {
	UPackage* RetPackage = CreatePackage(NULL, *InRelativeFilename);
	RetPackage->MarkPackageDirty();
	if (bLoadPackage) {
		RetPackage->FullyLoad();
	}
	return RetPackage;
}

void FUtuPluginAssetTypeProcessor::ProcessMaterial(FUtuPluginMaterial InUtuMaterial) {
	if (!InUtuMaterial.asset_relative_filename.StartsWith("Resources")) { // Default Unity Material
		// Format Paths
		TArray<FString> AssetNames = StartProcessAsset(InUtuMaterial, true);
		// Invalid Asset
		if (DeleteInvalidAssetIfNeeded(AssetNames, UMaterial::StaticClass())) {
			// Existing Asset
			UMaterial* Asset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
			LogAssetCreateOrNot(Asset);
			// Create Asset
			if (Asset == nullptr) {
				UPackage* Package = CreateAssetPackage(AssetNames[2], false);
				NewObject<UMaterialFactoryNew>()->FactoryCreateNew(UMaterial::StaticClass(), Package, FName(*AssetNames[1]), RF_Public | RF_Standalone, NULL, GWarn);
				Asset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
				LogAssetImportedOrFailed(Asset, AssetNames, "", "Material", { });
			}
			// Process Asset
			if (Asset != nullptr) {
				Asset->PreEditChange(NULL);
				UTU_LOG_L("        Building Material...");
				Asset->GetOutermost()->FullyLoad();
				Asset->MarkPackageDirty();
				// Parameters
				if (InUtuMaterial.shader_type == EUtuShaderType::Standard || InUtuMaterial.shader_type == EUtuShaderType::StandardSpecular) {
					if (InUtuMaterial.shader_type == EUtuShaderType::Standard) {
						UTU_LOG_L("            Shader Type: Standard");
					}
					else if (InUtuMaterial.shader_type == EUtuShaderType::StandardSpecular) {
						UTU_LOG_L("            Shader Type: Standard (Specular setup)");
					}
					{ // Blend Mode & Shading Model
						Asset->SetShadingModel(EMaterialShadingModel::MSM_DefaultLit);
						if (InUtuMaterial.shader_opacity == EUtuShaderOpacity::Opaque) {
							Asset->BlendMode = EBlendMode::BLEND_Opaque;
						}
						else if (InUtuMaterial.shader_opacity == EUtuShaderOpacity::Masked) {
							Asset->BlendMode = EBlendMode::BLEND_Masked;
						}
						else {
							Asset->BlendMode = EBlendMode::BLEND_Translucent;
						}
					}
					int H = -600;
					int V = -600;
					{ // Albedo
						UTexture2D* AlbedoTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.albedo_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Albedo = GetOrCreateTextureParameter(Asset, AlbedoTextureAsset, "Albedo", H, V, InUtuMaterial);
						UMaterialExpressionPanner* AlbedoPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.Z, InUtuMaterial.albedo_tiling_and_offset.W), "Albedo Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* AlbedoTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.X, InUtuMaterial.albedo_tiling_and_offset.Y), "Albedo TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* AlbedoPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Albedo Time", H - 300, V + 50, InUtuMaterial);
						UMaterialExpressionVectorParameter* AlbedoColor = GetOrCreateVectorParameter(Asset, HexToColor(InUtuMaterial.albedo_multiply_color), "Albedo Color", H, V + 200, InUtuMaterial);
						UMaterialExpressionMultiply* AlbedoMultiply = GetOrCreateMultiplyExpression(Asset, "Albedo Multiply With Color", H + 200, V + 100, InUtuMaterial);
						UMaterialExpressionMultiply* AlbedoAlphaMultiply = GetOrCreateMultiplyExpression(Asset, "Albedo Multiply With Color Alpha", H + 200, V + 200, InUtuMaterial);
						if (Albedo != nullptr && AlbedoColor != nullptr && AlbedoMultiply != nullptr && AlbedoAlphaMultiply != nullptr && AlbedoPanner != nullptr && AlbedoPannerTime != nullptr && AlbedoTexCoord != nullptr) {
							if (AlbedoTextureAsset == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
								Asset->GetEditorOnlyData()->Opacity.Connect(4, AlbedoColor);
								Asset->GetEditorOnlyData()->OpacityMask.Connect(4, AlbedoColor);
								Asset->GetEditorOnlyData()->BaseColor.Connect(0, AlbedoColor);
#else
								Asset->Opacity.Connect(4, AlbedoColor);
								Asset->OpacityMask.Connect(4, AlbedoColor);
								Asset->BaseColor.Connect(0, AlbedoColor);
#endif
							}
							else {
#if ENGINE_MAJOR_VERSION >= 5
								Asset->GetEditorOnlyData()->Opacity.Connect(0, AlbedoAlphaMultiply);
								Asset->GetEditorOnlyData()->OpacityMask.Connect(0, AlbedoAlphaMultiply);
								Asset->GetEditorOnlyData()->BaseColor.Connect(0, AlbedoMultiply);
#else
								Asset->Opacity.Connect(0, AlbedoAlphaMultiply);
								Asset->OpacityMask.Connect(0, AlbedoAlphaMultiply);
								Asset->BaseColor.Connect(0, AlbedoMultiply);
#endif

							}
							Albedo->ConnectExpression(&AlbedoMultiply->A, 0);
							AlbedoColor->ConnectExpression(&AlbedoMultiply->B, 0);
							Albedo->ConnectExpression(&AlbedoAlphaMultiply->A, 4);
							AlbedoColor->ConnectExpression(&AlbedoAlphaMultiply->B, 4);
							AlbedoPanner->ConnectExpression(&Albedo->Coordinates, 0);
							AlbedoPannerTime->ConnectExpression(&AlbedoPanner->Time, 0);
							AlbedoTexCoord->ConnectExpression(&AlbedoPanner->Coordinate, 0);
						}
					}
					H = -600;
					V = -200;
					{ // Specular
						UMaterialExpressionScalarParameter* Smoothness = GetOrCreateScalarParameter(Asset, 1 - InUtuMaterial.smoothness, "Smoothness", H, V + 300, InUtuMaterial);
						// Metallic Setup
						H = InUtuMaterial.shader_type == EUtuShaderType::Standard ? -600 : -1400;
						UTexture2D* MetallicTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.metallic_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Metallic = GetOrCreateTextureParameter(Asset, MetallicTextureAsset, "Metallic", H, V, InUtuMaterial);
						UMaterialExpressionPanner* MetallicPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.metallic_tiling_and_offset.Z, InUtuMaterial.metallic_tiling_and_offset.W), "Metallic Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* MetallicTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.metallic_tiling_and_offset.X, InUtuMaterial.metallic_tiling_and_offset.Y), "Metallic TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* MetallicPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Metallic Time", H - 300, V + 50, InUtuMaterial);
						UMaterialExpressionScalarParameter* MetallicIntensity = GetOrCreateScalarParameter(Asset, MetallicTextureAsset != nullptr ? ImportSettings.MetallicIntensityMultiplier : InUtuMaterial.metallic_intensity * ImportSettings.MetallicIntensityMultiplier, "Metallic Intensity", H, V + 200, InUtuMaterial);
						if (Metallic != nullptr && MetallicIntensity != nullptr && MetallicPanner != nullptr && MetallicPannerTime != nullptr && MetallicTexCoord != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
							if (MetallicTextureAsset == nullptr) {
								Asset->GetEditorOnlyData()->Metallic.Connect(0, MetallicIntensity);
							}
							else {
								Asset->GetEditorOnlyData()->Metallic.Connect(0, Metallic);
							}
							Asset->GetEditorOnlyData()->Specular.Connect(0, MetallicIntensity);
							Asset->GetEditorOnlyData()->Roughness.Connect(0, Smoothness);
#else
							if (MetallicTextureAsset == nullptr) {
								Asset->Metallic.Connect(0, MetallicIntensity);
							}
							else {
								Asset->Metallic.Connect(0, Metallic);
							}
							Asset->Specular.Connect(0, MetallicIntensity);
							Asset->Roughness.Connect(0, Smoothness);
#endif

							MetallicPanner->ConnectExpression(&Metallic->Coordinates, 0);
							MetallicPannerTime->ConnectExpression(&MetallicPanner->Time, 0);
							MetallicTexCoord->ConnectExpression(&MetallicPanner->Coordinate, 0);
						}
						// Specular Setup
						H = InUtuMaterial.shader_type == EUtuShaderType::Standard ? -1400 : -600;
						UTexture2D* SpecularTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.specular_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Specular = GetOrCreateTextureParameter(Asset, SpecularTextureAsset, "Specular", H, V, InUtuMaterial);
						UMaterialExpressionPanner* SpecularPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.specular_tiling_and_offset.Z, InUtuMaterial.specular_tiling_and_offset.W), "Specular Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* SpecularTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.specular_tiling_and_offset.X, InUtuMaterial.specular_tiling_and_offset.Y), "Specular TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* SpecularPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Specular Time", H - 300, V + 50, InUtuMaterial);
						UMaterialExpressionScalarParameter* SpecularIntensity = GetOrCreateScalarParameter(Asset, 1 - InUtuMaterial.specular_intensity * ImportSettings.SpecularIntensityMultiplier, "Specular Intensity", H, V + 200, InUtuMaterial);
						UMaterialExpressionVectorParameter* SpecularColor = GetOrCreateVectorParameter(Asset, HexToColor(InUtuMaterial.specular_color), "Specular Color - Not Supported By Unreal", -1300, V, InUtuMaterial);
						if (Specular != nullptr && SpecularIntensity != nullptr && SpecularPanner != nullptr && SpecularPannerTime != nullptr && SpecularTexCoord != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
							if (SpecularTextureAsset == nullptr) {
								Asset->GetEditorOnlyData()->Specular.Connect(0, SpecularIntensity);
							}
							else {
								Asset->GetEditorOnlyData()->Specular.Connect(0, Specular);
							}
							//Asset->Metallic.Connect(0, SpecularIntensity);
							Asset->GetEditorOnlyData()->Roughness.Connect(0, Smoothness);
#else
							if (SpecularTextureAsset == nullptr) {
								Asset->Specular.Connect(0, SpecularIntensity);
							}
							else {
								Asset->Specular.Connect(0, Specular);
							}
							//Asset->Metallic.Connect(0, SpecularIntensity);
							Asset->Roughness.Connect(0, Smoothness);
#endif
							SpecularPanner->ConnectExpression(&Specular->Coordinates, 0);
							SpecularPannerTime->ConnectExpression(&SpecularPanner->Time, 0);
							SpecularTexCoord->ConnectExpression(&SpecularPanner->Coordinate, 0);
						}
					}
					H = -600;
					V = 200;
					{ // Emissive
						UTexture2D* EmissiveTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.emission_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Emissive = GetOrCreateTextureParameter(Asset, EmissiveTextureAsset, "Emissive", H, V, InUtuMaterial);
						UMaterialExpressionMultiply* EmissiveTextureMultiply = GetOrCreateMultiplyExpression(Asset, "Emissive Multiply With Texture", H + 200, V + 100, InUtuMaterial);
						UMaterialExpressionScalarParameter* EmissiveTextureIntensity = GetOrCreateScalarParameter(Asset, ImportSettings.EmissiveIntensityMultiplier, "Emissive Texture Intensity", H, V + 200, InUtuMaterial);
						UMaterialExpressionPanner* EmissivePanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.emission_tiling_and_offset.Z, InUtuMaterial.emission_tiling_and_offset.W), "Emissive Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* EmissiveTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.emission_tiling_and_offset.X, InUtuMaterial.emission_tiling_and_offset.Y), "Emissive TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* EmissivePannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Emissive Time", H - 300, V + 50, InUtuMaterial);
						UMaterialExpressionMultiply* EmissiveMultiply = GetOrCreateMultiplyExpression(Asset, "Emissive Multiply With Color", H + 350, V + 200, InUtuMaterial);
						UMaterialExpressionVectorParameter* EmissiveColor = GetOrCreateVectorParameter(Asset, HexToColor(InUtuMaterial.emission_color), "Emissive Color", H, V + 300, InUtuMaterial);
						UMaterialExpressionScalarParameter* EmissiveColorIntensity = GetOrCreateScalarParameter(Asset, ImportSettings.EmissiveIntensityMultiplier, "Emissive Color Intensity", H, V + 500, InUtuMaterial);
						UMaterialExpressionMultiply* EmissiveColorIntensityMultiply = GetOrCreateMultiplyExpression(Asset, "Emissive Multiply With Color Intensity", H + 200, V + 400, InUtuMaterial);
						if (Emissive != nullptr && EmissiveColor != nullptr && EmissivePanner != nullptr && EmissivePannerTime != nullptr && EmissiveTextureMultiply != nullptr && EmissiveTextureIntensity != nullptr && EmissiveMultiply != nullptr && EmissiveTexCoord != nullptr && EmissiveColorIntensity != nullptr && EmissiveColorIntensityMultiply != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
							if (EmissiveTextureAsset == nullptr) {
								Asset->GetEditorOnlyData()->EmissiveColor.Connect(0, EmissiveColorIntensityMultiply);
							}
							else {
								Asset->GetEditorOnlyData()->EmissiveColor.Connect(0, EmissiveMultiply);
							}
#else
							if (EmissiveTextureAsset == nullptr) {
								Asset->EmissiveColor.Connect(0, EmissiveColorIntensityMultiply);
							}
							else {
								Asset->EmissiveColor.Connect(0, EmissiveMultiply);
							}
#endif
							Emissive->ConnectExpression(&EmissiveTextureMultiply->A, 0);
							EmissiveTextureIntensity->ConnectExpression(&EmissiveTextureMultiply->B, 0);
							EmissiveTextureMultiply->ConnectExpression(&EmissiveMultiply->A, 0);
							EmissiveColorIntensityMultiply->ConnectExpression(&EmissiveMultiply->B, 0);
							EmissiveColor->ConnectExpression(&EmissiveColorIntensityMultiply->A, 0);
							EmissiveColorIntensity->ConnectExpression(&EmissiveColorIntensityMultiply->B, 0);
							EmissivePanner->ConnectExpression(&Emissive->Coordinates, 0);
							EmissivePannerTime->ConnectExpression(&EmissivePanner->Time, 0);
							EmissiveTexCoord->ConnectExpression(&EmissivePanner->Coordinate, 0);
						}
					}
					H = -600;
					V = 800;
					{ // Normal
						UTexture2D* NormalTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.normal_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Normal = GetOrCreateTextureParameter(Asset, NormalTextureAsset, "Normal", H, V, InUtuMaterial);
						UMaterialExpressionPanner* NormalPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.normal_tiling_and_offset.Z, InUtuMaterial.normal_tiling_and_offset.W), "Normal Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* NormalTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.normal_tiling_and_offset.X, InUtuMaterial.normal_tiling_and_offset.Y), "Normal TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* NormalPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Normal Time", H - 300, V + 50, InUtuMaterial);
						UMaterialExpressionScalarParameter* NormalIntensity = GetOrCreateScalarParameter(Asset, InUtuMaterial.normal_intensity * ImportSettings.NormalIntensityMultiplier, "Normal Intensity", H, V + 200, InUtuMaterial);
						UMaterialExpressionMultiply* NormalMultiply = GetOrCreateMultiplyExpression(Asset, "Normal Multiply With Intensity", H + 200, V + 100, InUtuMaterial);
						if (Normal != nullptr && NormalIntensity != nullptr && NormalMultiply != nullptr && NormalPanner != nullptr && NormalPannerTime != nullptr && NormalTexCoord != nullptr) {
							Normal->SamplerType = EMaterialSamplerType::SAMPLERTYPE_Normal;
							Normal->ConnectExpression(&NormalMultiply->A, 0);
							NormalIntensity->ConnectExpression(&NormalMultiply->B, 0);
#if ENGINE_MAJOR_VERSION >= 5
							if (NormalTextureAsset == nullptr) {
								Asset->GetEditorOnlyData()->Normal.Expression = nullptr;
							}
							else {
								NormalTextureAsset->CompressionSettings = TC_Normalmap;
								NormalTextureAsset->SRGB = false;
								NormalTextureAsset->LODGroup = TEXTUREGROUP_WorldNormalMap;
								Asset->GetEditorOnlyData()->Normal.Connect(0, NormalMultiply);
							}
#else
							if (NormalTextureAsset == nullptr) {
								Asset->Normal.Expression = nullptr;
							}
							else {
								NormalTextureAsset->CompressionSettings = TC_Normalmap;
								NormalTextureAsset->SRGB = false;
								NormalTextureAsset->LODGroup = TEXTUREGROUP_WorldNormalMap;
								Asset->Normal.Connect(0, NormalMultiply);
							}
#endif
							NormalPanner->ConnectExpression(&Normal->Coordinates, 0);
							NormalPannerTime->ConnectExpression(&NormalPanner->Time, 0);
							NormalTexCoord->ConnectExpression(&NormalPanner->Coordinate, 0);
						}
					}
					H = -600;
					V = 1200;
					{ // Occlusion
						UTexture2D* OcclusionTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.occlusion_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Occlusion = GetOrCreateTextureParameter(Asset, OcclusionTextureAsset, "Occlusion", H, V, InUtuMaterial);
						UMaterialExpressionPanner* OcclusionPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.occlusion_tiling_and_offset.Z, InUtuMaterial.occlusion_tiling_and_offset.W), "Occlusion Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* OcclusionTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.occlusion_tiling_and_offset.X, InUtuMaterial.occlusion_tiling_and_offset.Y), "Occlusion TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* OcclusionPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Occlusion Time", H - 300, V + 50, InUtuMaterial);
						UMaterialExpressionScalarParameter* OcclusionIntensity = GetOrCreateScalarParameter(Asset, InUtuMaterial.occlusion_intensity * ImportSettings.OcclusionIntensityMultiplier, "Occlusion Intensity", H, V + 200, InUtuMaterial);
						UMaterialExpressionMultiply* OcclusionMultiply = GetOrCreateMultiplyExpression(Asset, "Occlusion Multiply With Intensity", H + 200, V + 100, InUtuMaterial);
						if (Occlusion != nullptr && OcclusionIntensity != nullptr && OcclusionMultiply != nullptr && OcclusionPanner != nullptr && OcclusionPannerTime != nullptr && OcclusionTexCoord != nullptr) {
							Occlusion->ConnectExpression(&OcclusionMultiply->A, 0);
							OcclusionIntensity->ConnectExpression(&OcclusionMultiply->B, 0);
#if ENGINE_MAJOR_VERSION >= 5
							if (OcclusionTextureAsset == nullptr) {
								Asset->GetEditorOnlyData()->Normal.Expression = nullptr;
							}
							else {
								Asset->GetEditorOnlyData()->AmbientOcclusion.Connect(0, OcclusionMultiply);
							}
#else
							if (OcclusionTextureAsset == nullptr) {
								Asset->Normal.Expression = nullptr;
							}
							else {
								Asset->AmbientOcclusion.Connect(0, OcclusionMultiply);
							}
#endif
							OcclusionPanner->ConnectExpression(&Occlusion->Coordinates, 0);
							OcclusionPannerTime->ConnectExpression(&OcclusionPanner->Time, 0);
							OcclusionTexCoord->ConnectExpression(&OcclusionPanner->Coordinate, 0);
						}
					}
					H = 600;
					V = -200;
					{ // Unknown Textures
						UTexture2D* HeightTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.height_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* Height = GetOrCreateTextureParameter(Asset, HeightTextureAsset, "Height", H, V, InUtuMaterial);
						UMaterialExpressionPanner* HeightPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.height_tiling_and_offset.Z, InUtuMaterial.height_tiling_and_offset.W), "Height Panner", H - 150, V, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* HeightTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.height_tiling_and_offset.X, InUtuMaterial.height_tiling_and_offset.Y), "Height TexCoord", H - 300, V, InUtuMaterial);
						UMaterialExpressionScalarParameter* HeightPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Height Time", H - 300, V + 50, InUtuMaterial);
						if (HeightPanner != nullptr && HeightPannerTime != nullptr && HeightTexCoord != nullptr) {
							HeightPanner->ConnectExpression(&Height->Coordinates, 0);
							HeightPannerTime->ConnectExpression(&HeightPanner->Time, 0);
							HeightTexCoord->ConnectExpression(&HeightPanner->Coordinate, 0);
						}
						if (HeightTextureAsset != nullptr) {
							UTU_LOG_W("                    Type: Height");
							UTU_LOG_W("                        Output NOT Plugged. No equivalent input is known yet. You will have to plug it manually.");
						}
						UTexture2D* DetailMaskTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.detail_mask_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* DetailMask = GetOrCreateTextureParameter(Asset, DetailMaskTextureAsset, "Detail Mask", H, V + 200, InUtuMaterial);
						UMaterialExpressionPanner* DetailMaskPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.detail_mask_tiling_and_offset.Z, InUtuMaterial.detail_mask_tiling_and_offset.W), "DetailMask Panner", H - 150, V + 200, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* DetailMaskTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.detail_mask_tiling_and_offset.X, InUtuMaterial.detail_mask_tiling_and_offset.Y), "DetailMask TexCoord", H - 300, V + 200, InUtuMaterial);
						UMaterialExpressionScalarParameter* DetailMaskPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "DetailMask Time", H - 300, V + 250, InUtuMaterial);
						if (DetailMaskPanner != nullptr && DetailMaskPannerTime != nullptr && DetailMaskTexCoord != nullptr) {
							DetailMaskPanner->ConnectExpression(&DetailMask->Coordinates, 0);
							DetailMaskPannerTime->ConnectExpression(&DetailMaskPanner->Time, 0);
							DetailMaskTexCoord->ConnectExpression(&DetailMaskPanner->Coordinate, 0);
						}
						if (DetailMaskTextureAsset != nullptr) {
							UTU_LOG_W("                    Type: Detail Mask");
							UTU_LOG_W("                        Output NOT Plugged. No equivalent input is known yet. You will have to plug it manually.");
						}
						UTexture2D* DetailAlbedoTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.detail_albedo_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* DetailAlbedo = GetOrCreateTextureParameter(Asset, DetailAlbedoTextureAsset, "Detail Albedo", H, V + 400, InUtuMaterial);
						UMaterialExpressionPanner* DetailAlbedoPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.detail_albedo_tiling_and_offset.Z, InUtuMaterial.detail_albedo_tiling_and_offset.W), "DetailAlbedo Panner", H - 150, V + 400, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* DetailAlbedoTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.detail_albedo_tiling_and_offset.X, InUtuMaterial.detail_albedo_tiling_and_offset.Y), "DetailAlbedo TexCoord", H - 300, V + 400, InUtuMaterial);
						UMaterialExpressionScalarParameter* DetailAlbedoPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "DetailAlbedo Time", H - 300, V + 450, InUtuMaterial);
						if (DetailAlbedoPanner != nullptr && DetailAlbedoPannerTime != nullptr && DetailAlbedoTexCoord != nullptr) {
							DetailAlbedoPanner->ConnectExpression(&DetailAlbedo->Coordinates, 0);
							DetailAlbedoPannerTime->ConnectExpression(&DetailAlbedoPanner->Time, 0);
							DetailAlbedoTexCoord->ConnectExpression(&DetailAlbedoPanner->Coordinate, 0);
						}
						if (DetailAlbedoTextureAsset != nullptr) {
							UTU_LOG_W("                    Type: Detail Albedo");
							UTU_LOG_W("                        Output NOT Plugged. No equivalent input is known yet. You will have to plug it manually.");
						}
						UTexture2D* DetailNormalTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.detail_normal_relative_filename);
						UMaterialExpressionTextureSampleParameter2D* DetailNormal = GetOrCreateTextureParameter(Asset, DetailNormalTextureAsset, "Detail Normal", H, V + 600, InUtuMaterial);
						UMaterialExpressionPanner* DetailNormalPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.detail_normal_tiling_and_offset.Z, InUtuMaterial.detail_normal_tiling_and_offset.W), "DetailNormal Panner", H - 150, V + 600, InUtuMaterial);
						UMaterialExpressionTextureCoordinate* DetailNormalTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.detail_normal_tiling_and_offset.X, InUtuMaterial.detail_normal_tiling_and_offset.Y), "DetailNormal TexCoord", H - 300, V + 600, InUtuMaterial);
						UMaterialExpressionScalarParameter* DetailNormalPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "DetailNormal Time", H - 300, V + 650, InUtuMaterial);
						UMaterialExpressionScalarParameter* DetailNormalIntensity = GetOrCreateScalarParameter(Asset, InUtuMaterial.detail_normal_intensity * ImportSettings.NormalIntensityMultiplier, "Detail Normal Intensity", H, V + 800, InUtuMaterial);
						UMaterialExpressionMultiply* DetailNormalMultiply = GetOrCreateMultiplyExpression(Asset, "Detail Normal Multiply With Intensity", H + 200, V + 700, InUtuMaterial);
						if (DetailNormal != nullptr && DetailNormalIntensity != nullptr && DetailNormalMultiply != nullptr && DetailNormalPanner != nullptr && DetailNormalPannerTime != nullptr && DetailNormalTexCoord != nullptr) {
							DetailNormal->ConnectExpression(&DetailNormalMultiply->A, 0);
							DetailNormalIntensity->ConnectExpression(&DetailNormalMultiply->B, 0);
							DetailNormalPanner->ConnectExpression(&DetailNormal->Coordinates, 0);
							DetailNormalPannerTime->ConnectExpression(&DetailNormalPanner->Time, 0);
							DetailNormalTexCoord->ConnectExpression(&DetailNormalPanner->Coordinate, 0);
						}
						if (DetailNormalTextureAsset != nullptr) {
							UTU_LOG_W("                    Type: Detail Normal");
							UTU_LOG_W("                        Output NOT Plugged. No equivalent input is known yet. You will have to plug it manually.");
						}
					}
				}
				else if (InUtuMaterial.shader_type == EUtuShaderType::UnlitColor) {
					UTU_LOG_L("            Shader Type: Unlit/Color");
					UMaterialExpressionVectorParameter* Param = GetOrCreateVectorParameter(Asset, HexToColor(InUtuMaterial.albedo_multiply_color), "Color", -300, 300, InUtuMaterial);
#if ENGINE_MAJOR_VERSION >= 5
					if (Param != nullptr) {
						Asset->GetEditorOnlyData()->EmissiveColor.Connect(0, Param);
					}
#else
					if (Param != nullptr) {
						Asset->EmissiveColor.Connect(0, Param);
					}
#endif
					Asset->BlendMode = EBlendMode::BLEND_Opaque;
					Asset->SetShadingModel(EMaterialShadingModel::MSM_Unlit);
				}
				else if (InUtuMaterial.shader_type == EUtuShaderType::UnlitTexture || InUtuMaterial.shader_type == EUtuShaderType::UnlitTransparent || InUtuMaterial.shader_type == EUtuShaderType::UnlitCutout) {
					if (InUtuMaterial.shader_type == EUtuShaderType::UnlitTexture) {
						UTU_LOG_L("            Shader Type: Unlit/Texture");
						Asset->BlendMode = EBlendMode::BLEND_Opaque;
					}
					else if (InUtuMaterial.shader_type == EUtuShaderType::UnlitTransparent) {
						UTU_LOG_L("            Shader Type: Unlit/Transparent");
						Asset->BlendMode = EBlendMode::BLEND_Translucent;
					}
					else if (InUtuMaterial.shader_type == EUtuShaderType::UnlitCutout){
						UTU_LOG_L("            Shader Type: Unlit/Transparent Cutout");
						Asset->BlendMode = EBlendMode::BLEND_Masked;
					}
					UMaterialExpressionTextureSampleParameter2D* Albedo = GetOrCreateTextureParameter(Asset, GetTextureFromUnityRelativeFilename(InUtuMaterial.albedo_relative_filename), "Albedo", -300, 300, InUtuMaterial);
					UMaterialExpressionPanner* AlbedoPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.Z, InUtuMaterial.albedo_tiling_and_offset.W), "Albedo Panner", -450, 300, InUtuMaterial);
					UMaterialExpressionTextureCoordinate* AlbedoTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.X, InUtuMaterial.albedo_tiling_and_offset.Y), "Albedo TexCoord", -600, 300, InUtuMaterial);
					UMaterialExpressionScalarParameter* AlbedoPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Albedo Time", -600, 350, InUtuMaterial);
					if (Albedo != nullptr && AlbedoPanner != nullptr && AlbedoPannerTime != nullptr && AlbedoTexCoord != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
						Asset->GetEditorOnlyData()->EmissiveColor.Connect(0, Albedo);
						if (InUtuMaterial.shader_type == EUtuShaderType::UnlitTransparent) {
							Asset->GetEditorOnlyData()->Opacity.Connect(4, Albedo);
						}
						else if (InUtuMaterial.shader_type == EUtuShaderType::UnlitCutout) {
							Asset->GetEditorOnlyData()->OpacityMask.Connect(4, Albedo);
						}
#else
						Asset->EmissiveColor.Connect(0, Albedo);
						if (InUtuMaterial.shader_type == EUtuShaderType::UnlitTransparent) {
							Asset->Opacity.Connect(4, Albedo);
						}
						else if (InUtuMaterial.shader_type == EUtuShaderType::UnlitCutout) {
							Asset->OpacityMask.Connect(4, Albedo);
						}
#endif
						AlbedoPanner->ConnectExpression(&Albedo->Coordinates, 0);
						AlbedoPannerTime->ConnectExpression(&AlbedoPanner->Time, 0);
						AlbedoTexCoord->ConnectExpression(&AlbedoPanner->Coordinate, 0);
					}
					Asset->SetShadingModel(EMaterialShadingModel::MSM_Unlit);
				}
				else if (InUtuMaterial.shader_type == EUtuShaderType::LegacyDiffuse) {
					UTU_LOG_L("            Shader Type: Legacy/Diffuse");
					{ // Blend Mode & Shading Model
						Asset->SetShadingModel(EMaterialShadingModel::MSM_DefaultLit);
						Asset->BlendMode = EBlendMode::BLEND_Opaque;
					}
					int H = -300;
					int V = -300;
					UTexture2D* AlbedoTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.albedo_relative_filename);
					UMaterialExpressionTextureSampleParameter2D* Albedo = GetOrCreateTextureParameter(Asset, AlbedoTextureAsset, "Albedo", H, V, InUtuMaterial);
					UMaterialExpressionPanner* AlbedoPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.Z, InUtuMaterial.albedo_tiling_and_offset.W), "Albedo Panner", H - 150, V, InUtuMaterial);
					UMaterialExpressionTextureCoordinate* AlbedoTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.X, InUtuMaterial.albedo_tiling_and_offset.Y), "Albedo TexCoord", H - 300, V, InUtuMaterial);
					UMaterialExpressionScalarParameter* AlbedoPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Albedo Time", H - 300, V + 50, InUtuMaterial);
					UMaterialExpressionVectorParameter* AlbedoColor = GetOrCreateVectorParameter(Asset, HexToColor(InUtuMaterial.albedo_multiply_color), "Albedo Color", H, V + 200, InUtuMaterial);
					UMaterialExpressionMultiply* AlbedoMultiply = GetOrCreateMultiplyExpression(Asset, "Albedo Multiply With Color", H + 200, V + 100, InUtuMaterial);
					UMaterialExpressionScalarParameter* MetallicSpecularRoughnessIntensity = GetOrCreateScalarParameter(Asset, 0.0f, "Metallic Specular Roughness Intensity", H, V + 600, InUtuMaterial);
					if (Albedo != nullptr && AlbedoColor != nullptr && AlbedoMultiply != nullptr && AlbedoPanner != nullptr && AlbedoPannerTime != nullptr && AlbedoTexCoord != nullptr && MetallicSpecularRoughnessIntensity != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
						if (AlbedoTextureAsset == nullptr) {
							Asset->GetEditorOnlyData()->BaseColor.Connect(0, AlbedoColor);
						}
						else {
							Asset->GetEditorOnlyData()->BaseColor.Connect(0, AlbedoMultiply);
						}
						Albedo->ConnectExpression(&AlbedoMultiply->A, 0);
						AlbedoColor->ConnectExpression(&AlbedoMultiply->B, 0);
						AlbedoPanner->ConnectExpression(&Albedo->Coordinates, 0);
						AlbedoPannerTime->ConnectExpression(&AlbedoPanner->Time, 0);
						AlbedoTexCoord->ConnectExpression(&AlbedoPanner->Coordinate, 0);
						Asset->GetEditorOnlyData()->Metallic.Connect(0, MetallicSpecularRoughnessIntensity);
						Asset->GetEditorOnlyData()->Specular.Connect(0, MetallicSpecularRoughnessIntensity);
						Asset->GetEditorOnlyData()->Roughness.Connect(0, MetallicSpecularRoughnessIntensity);
#else
						if (AlbedoTextureAsset == nullptr) {
							Asset->BaseColor.Connect(0, AlbedoColor);
						}
						else {
							Asset->BaseColor.Connect(0, AlbedoMultiply);
						}
						Albedo->ConnectExpression(&AlbedoMultiply->A, 0);
						AlbedoColor->ConnectExpression(&AlbedoMultiply->B, 0);
						AlbedoPanner->ConnectExpression(&Albedo->Coordinates, 0);
						AlbedoPannerTime->ConnectExpression(&AlbedoPanner->Time, 0);
						AlbedoTexCoord->ConnectExpression(&AlbedoPanner->Coordinate, 0);
						Asset->Metallic.Connect(0, MetallicSpecularRoughnessIntensity);
						Asset->Specular.Connect(0, MetallicSpecularRoughnessIntensity);
						Asset->Roughness.Connect(0, MetallicSpecularRoughnessIntensity);
#endif
					}
				}
				else if (InUtuMaterial.shader_type == EUtuShaderType::MobileDiffuse) {
					UTU_LOG_L("            Shader Type: Mobile/Diffuse");
					{ // Blend Mode & Shading Model
						Asset->SetShadingModel(EMaterialShadingModel::MSM_DefaultLit);
						Asset->BlendMode = EBlendMode::BLEND_Opaque;
					}
					int H = -300;
					int V = -300;
					UTexture2D* AlbedoTextureAsset = GetTextureFromUnityRelativeFilename(InUtuMaterial.albedo_relative_filename);
					UMaterialExpressionTextureSampleParameter2D* Albedo = GetOrCreateTextureParameter(Asset, AlbedoTextureAsset, "Albedo", H, V, InUtuMaterial);
					UMaterialExpressionPanner* AlbedoPanner = GetOrCreatePannerExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.Z, InUtuMaterial.albedo_tiling_and_offset.W), "Albedo Panner", H - 150, V, InUtuMaterial);
					UMaterialExpressionTextureCoordinate* AlbedoTexCoord = GetOrCreateTexCoordExpression(Asset, FVector2D(InUtuMaterial.albedo_tiling_and_offset.X, InUtuMaterial.albedo_tiling_and_offset.Y), "Albedo TexCoord", H - 300, V, InUtuMaterial);
					UMaterialExpressionScalarParameter* AlbedoPannerTime = GetOrCreateScalarParameter(Asset, ImportSettings.TexturesPannerTime, "Albedo Time", H - 300, V + 50, InUtuMaterial);
					if (Albedo != nullptr && AlbedoPanner != nullptr && AlbedoPannerTime != nullptr && AlbedoTexCoord != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
						if (AlbedoTextureAsset != nullptr) {
							Asset->GetEditorOnlyData()->BaseColor.Connect(0, Albedo);
						}
#else
						if (AlbedoTextureAsset != nullptr) {
							Asset->BaseColor.Connect(0, Albedo);
						}
#endif
						AlbedoPanner->ConnectExpression(&Albedo->Coordinates, 0);
						AlbedoPannerTime->ConnectExpression(&AlbedoPanner->Time, 0);
						AlbedoTexCoord->ConnectExpression(&AlbedoPanner->Coordinate, 0);
					}
				}
				else /*if (InUtuMaterial.shader_type == EUtuShaderType::Unsupported)*/ {
					UTU_LOG_W("            Shader Type: Unsupported");
					UTU_LOG_W("                This material was using an unsupported shader in Unity. Will still add all the referenced textures into it, but you will need to connect them manually.");
					UTU_LOG_W("                Material relative filename: " + InUtuMaterial.asset_relative_filename);
					UTU_LOG_W("                Supported shaders are: Standard, Standard (Specular setup), Unlit/Color, Unlit/Texture, Unlit/Transparent, Unlit/Transparent Cutout, Mobile/Diffuse, Mobile/Unlit (Supports Lightmap) & Legacy Shaders/Diffuse");
					int TextureOffsetMultiplier = -3;
					for (int x = 0; x < InUtuMaterial.material_textures_relative_filenames_for_unsupported_shaders.Num(); x++) {
						if (InUtuMaterial.material_textures_relative_filenames_for_unsupported_shaders[x] != "") {
							UMaterialExpressionTextureSampleParameter2D* Param = GetOrCreateTextureParameter(Asset, GetTextureFromUnityRelativeFilename(InUtuMaterial.material_textures_relative_filenames_for_unsupported_shaders[x]), *FString::FromInt(x), -300, 250 * TextureOffsetMultiplier, InUtuMaterial);
							// Do not connect these parameters!
							TextureOffsetMultiplier++;
						}
					}
					UTU_LOG_L("            Adding default color for unsupported material.");
					UMaterialExpressionVectorParameter* Param = GetOrCreateVectorParameter(Asset, FLinearColor(1.0f, 0.0f, 0.5f, 1.0f), "DefaultColorForUnsupportedShaderType", 0, -500, InUtuMaterial);
					if (Param != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
						Asset->GetEditorOnlyData()->EmissiveColor.Connect(0, Param);
#else
						Asset->EmissiveColor.Connect(0, Param);
#endif
					}
					Asset->BlendMode = EBlendMode::BLEND_Opaque;
					Asset->SetShadingModel(EMaterialShadingModel::MSM_Unlit);
				}
				Asset->PostEditChange();
				FGlobalComponentReregisterContext RecreateComponents;
			}
		}
	}
}

FLinearColor FUtuPluginAssetTypeProcessor::HexToColor(FString InHex) {
	return FLinearColor(FColor::FromHex(InHex));
}


UTexture2D* FUtuPluginAssetTypeProcessor::GetTextureFromUnityRelativeFilename(FString InUnityRelativeFilename) {
	if (InUnityRelativeFilename != "") {
		TArray<FString> TexNames = FormatRelativeFilenameForUnreal(InUnityRelativeFilename);
		UTU_LOG_L("            Texture: " + TexNames[2]);
		UTexture2D* TextureAsset = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, *TexNames[2]));
		if (TextureAsset == nullptr) {
			UTU_LOG_W("                Failed to associate texture because it doesn't exists: '" + TexNames[2] + "'");
		}
		return TextureAsset;
	}
	UTU_LOG_L("            Texture: None");
	return nullptr;
}


UMaterialExpressionTextureSampleParameter2D* FUtuPluginAssetTypeProcessor::GetOrCreateTextureParameter(UMaterial* InMaterial, UTexture* InTexture, FName InParamName, int InPosX, int InPosY, FUtuPluginMaterial InUtuMaterial) {
	UMaterialExpressionTextureSampleParameter2D* Ret = nullptr;
	UTU_LOG_L("                Texture Parameter: " + InParamName.ToString());
	if (InMaterial != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
		for (UMaterialExpression* Exp : InMaterial->GetExpressions()) {
#else
		for (UMaterialExpression* Exp : InMaterial->Expressions) {
#endif
			UMaterialExpressionTextureSampleParameter2D* Parameter = Cast<UMaterialExpressionTextureSampleParameter2D>(Exp);
			if (Parameter != nullptr && Parameter->ParameterName == InParamName) {
				UTU_LOG_L("                    Texture Parameter found in material.");
				Ret = Parameter;
				break;
			}
		}
		if (Ret == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			Ret = Cast<UMaterialExpressionTextureSampleParameter2D>(UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, UMaterialExpressionTextureSampleParameter2D::StaticClass()));
			if (Ret != nullptr) {
				UTU_LOG_L("                    Texture Parameter added into material.");
				Ret->ParameterName = InParamName;
				InMaterial->AddExpressionParameter(Ret, InMaterial->EditorParameters);
			}
			else {
				UTU_LOG_E("                    Failed to create Texture Parameter expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#else
			Ret = NewObject<UMaterialExpressionTextureSampleParameter2D>(InMaterial);
			if (Ret != nullptr) {
				UTU_LOG_L("                    Texture Parameter added into material.");
				Ret->ParameterName = InParamName;
				InMaterial->Expressions.Add(Ret);
			}
			else {
				UTU_LOG_E("                    Failed to create Texture Parameter expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#endif
		}
		if (Ret != nullptr) {
			Ret->Texture = InTexture;
			Ret->SamplerType = SAMPLERTYPE_Color;
			Ret->MaterialExpressionEditorX = InPosX;
			Ret->MaterialExpressionEditorY = InPosY;
		}
	}
	return Ret;
}

UMaterialExpressionScalarParameter* FUtuPluginAssetTypeProcessor::GetOrCreateScalarParameter(UMaterial* InMaterial, float InValue, FName InParamName, int InPosX, int InPosY, FUtuPluginMaterial InUtuMaterial) {
	UMaterialExpressionScalarParameter* Ret = nullptr;
	UTU_LOG_L("                Scalar Parameter: " + InParamName.ToString());
	if (InMaterial != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
		for (UMaterialExpression* Exp : InMaterial->GetExpressions()) {
#else
		for (UMaterialExpression* Exp : InMaterial->Expressions) {
#endif
			UMaterialExpressionScalarParameter* Parameter = Cast<UMaterialExpressionScalarParameter>(Exp);
			if (Parameter != nullptr && Parameter->ParameterName == InParamName) {
				UTU_LOG_L("                    Scalar Parameter found in material.");
				Ret = Parameter;
				break;
			}
		}
		if (Ret == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			Ret = Cast<UMaterialExpressionScalarParameter>(UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, UMaterialExpressionScalarParameter::StaticClass()));
			if (Ret != nullptr) {
				UTU_LOG_L("                    Scalar Parameter added into material.");
				Ret->ParameterName = InParamName;
				InMaterial->AddExpressionParameter(Ret, InMaterial->EditorParameters);
			}
			else {
				UTU_LOG_E("                    Failed to create Scalar Parameter expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#else
			Ret = NewObject<UMaterialExpressionScalarParameter>(InMaterial);
			if (Ret != nullptr) {
				UTU_LOG_L("                    Scalar Parameter added into material.");
				Ret->ParameterName = InParamName;
				InMaterial->Expressions.Add(Ret);
			}
			else {
				UTU_LOG_E("                    Failed to create Scalar Parameter expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#endif
		}
		if (Ret != nullptr) {
			Ret->DefaultValue = InValue;
			Ret->MaterialExpressionEditorX = InPosX;
			Ret->MaterialExpressionEditorY = InPosY;
		}
	}
	return Ret;
}

UMaterialExpressionVectorParameter* FUtuPluginAssetTypeProcessor::GetOrCreateVectorParameter(UMaterial* InMaterial, FLinearColor InColor, FName InParamName, int InPosX, int InPosY, FUtuPluginMaterial InUtuMaterial) {
	UMaterialExpressionVectorParameter* Ret = nullptr;
	UTU_LOG_L("                Vector Parameter: " + InParamName.ToString());
	if (InMaterial != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
		for (UMaterialExpression* Exp : InMaterial->GetExpressions()) {
#else
		for (UMaterialExpression* Exp : InMaterial->Expressions) {
#endif
			UMaterialExpressionVectorParameter* Parameter = Cast<UMaterialExpressionVectorParameter>(Exp);
			if (Parameter != nullptr && Parameter->ParameterName == InParamName) {
				UTU_LOG_L("                    Vector Parameter found in material.");
				Ret = Parameter;
				break;
			}
		}
		if (Ret == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			Ret = Cast<UMaterialExpressionVectorParameter>(UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, UMaterialExpressionVectorParameter::StaticClass()));
			if (Ret != nullptr) {
				UTU_LOG_L("                    Vector Parameter added into material.");
				Ret->ParameterName = InParamName;
				InMaterial->AddExpressionParameter(Ret, InMaterial->EditorParameters);
			}
			else {
				UTU_LOG_E("                    Failed to create Vector Parameter expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#else
			Ret = NewObject<UMaterialExpressionVectorParameter>(InMaterial);
			if (Ret != nullptr) {
				UTU_LOG_L("                    Vector Parameter added into material.");
				Ret->ParameterName = InParamName;
				InMaterial->Expressions.Add(Ret);
			}
			else {
				UTU_LOG_E("                    Failed to create Vector Parameter expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#endif
		}
		if (Ret != nullptr) {
			Ret->DefaultValue = InColor;
			Ret->MaterialExpressionEditorX = InPosX;
			Ret->MaterialExpressionEditorY = InPosY;
		}
	}
	return Ret;
}

UMaterialExpressionMultiply* FUtuPluginAssetTypeProcessor::GetOrCreateMultiplyExpression(UMaterial* InMaterial, FString InExpressionName, int InPosX, int InPosY, FUtuPluginMaterial InUtuMaterial) {
	UMaterialExpressionMultiply* Ret = nullptr;
	UTU_LOG_L("                Multiply Expression: " + InExpressionName);
	if (InMaterial != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
		for (UMaterialExpression* Exp : InMaterial->GetExpressions()) {
#else
		for (UMaterialExpression* Exp : InMaterial->Expressions) {
#endif
			UMaterialExpressionMultiply* Expression = Cast<UMaterialExpressionMultiply>(Exp);
			if (Expression != nullptr && Expression->Desc == InExpressionName) {
				UTU_LOG_L("                    Multiply Expression found in material.");
				Ret = Expression;
				break;
			}
		}
		if (Ret == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			Ret = Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, UMaterialExpressionMultiply::StaticClass()));
			if (Ret != nullptr) {
				UTU_LOG_L("                   Multiply Expression added into material.");
				Ret->Desc = InExpressionName;
				InMaterial->AddExpressionParameter(Ret, InMaterial->EditorParameters);
			}
			else {
				UTU_LOG_E("                    Failed to createMultiply Expression expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#else
			Ret = NewObject<UMaterialExpressionMultiply>(InMaterial);
			if (Ret != nullptr) {
				UTU_LOG_L("                   Multiply Expression added into material.");
				Ret->Desc = InExpressionName;
				InMaterial->Expressions.Add(Ret);
			}
			else {
				UTU_LOG_E("                    Failed to createMultiply Expression expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#endif
		}
		if (Ret != nullptr) {
			Ret->MaterialExpressionEditorX = InPosX;
			Ret->MaterialExpressionEditorY = InPosY;
		}
	}
	return Ret;
}

UMaterialExpressionPanner* FUtuPluginAssetTypeProcessor::GetOrCreatePannerExpression(UMaterial* InMaterial, FVector2D InValue, FString InExpressionName, int InPosX, int InPosY, FUtuPluginMaterial InUtuMaterial) {
	UMaterialExpressionPanner* Ret = nullptr;
	UTU_LOG_L("                Panner Expression: " + InExpressionName);
	if (InMaterial != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
		for (UMaterialExpression* Exp : InMaterial->GetExpressions()) {
#else
		for (UMaterialExpression* Exp : InMaterial->Expressions) {
#endif
			UMaterialExpressionPanner* Expression = Cast<UMaterialExpressionPanner>(Exp);
			if (Expression != nullptr && Expression->Desc == InExpressionName) {
				UTU_LOG_L("                    Panner Expression found in material.");
				Ret = Expression;
				break;
			}
		}
		if (Ret == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			Ret = Cast<UMaterialExpressionPanner>(UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, UMaterialExpressionPanner::StaticClass()));
			if (Ret != nullptr) {
				UTU_LOG_L("                   Panner Expression added into material.");
				Ret->Desc = InExpressionName;
				InMaterial->AddExpressionParameter(Ret, InMaterial->EditorParameters);
			}
			else {
				UTU_LOG_E("                    Failed to create Panner Expression expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#else
			Ret = NewObject<UMaterialExpressionPanner>(InMaterial);
			if (Ret != nullptr) {
				UTU_LOG_L("                   Panner Expression added into material.");
				Ret->Desc = InExpressionName;
				InMaterial->Expressions.Add(Ret);
			}
			else {
				UTU_LOG_E("                    Failed to create Panner Expression expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#endif
		}
		if (Ret != nullptr) {
			Ret->MaterialExpressionEditorX = InPosX;
			Ret->MaterialExpressionEditorY = InPosY;
			Ret->SpeedX = InValue.X;
			Ret->SpeedY = InValue.Y;
		}
	}
	return Ret;
}

UMaterialExpressionTextureCoordinate* FUtuPluginAssetTypeProcessor::GetOrCreateTexCoordExpression(UMaterial* InMaterial, FVector2D InValue, FString InExpressionName, int InPosX, int InPosY, FUtuPluginMaterial InUtuMaterial) {
	UMaterialExpressionTextureCoordinate* Ret = nullptr;
	UTU_LOG_L("                TexCoord Expression: " + InExpressionName);
	if (InMaterial != nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
		for (UMaterialExpression* Exp : InMaterial->GetExpressions()) {
#else
		for (UMaterialExpression* Exp : InMaterial->Expressions) {
#endif
			UMaterialExpressionTextureCoordinate* Expression = Cast<UMaterialExpressionTextureCoordinate>(Exp);
			if (Expression != nullptr && Expression->Desc == InExpressionName) {
				UTU_LOG_L("                    TexCoord Expression found in material.");
				Ret = Expression;
				break;
			}
		}
		if (Ret == nullptr) {
#if ENGINE_MAJOR_VERSION >= 5
			Ret = Cast<UMaterialExpressionTextureCoordinate>(UMaterialEditingLibrary::CreateMaterialExpression(InMaterial, UMaterialExpressionTextureCoordinate::StaticClass()));
			if (Ret != nullptr) {
				UTU_LOG_L("                   TexCoord Expression added into material.");
				Ret->Desc = InExpressionName;
				InMaterial->AddExpressionParameter(Ret, InMaterial->EditorParameters);
			}
			else {
				UTU_LOG_E("                    Failed to create TexCoord Expression expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#else
			Ret = NewObject<UMaterialExpressionTextureCoordinate>(InMaterial);
			if (Ret != nullptr) {
				UTU_LOG_L("                   TexCoord Expression added into material.");
				Ret->Desc = InExpressionName;
				InMaterial->Expressions.Add(Ret);
			}
			else {
				UTU_LOG_E("                    Failed to create TexCoord Expression expression for Material: '" + InUtuMaterial.asset_relative_filename + "'. This is not normal and should never happen.");
			}
#endif
		}
		if (Ret != nullptr) {
			Ret->MaterialExpressionEditorX = InPosX;
			Ret->MaterialExpressionEditorY = InPosY;
			Ret->UTiling = InValue.X;
			Ret->VTiling = InValue.Y;
		}
	}
	return Ret;
}

void FUtuPluginAssetTypeProcessor::ProcessTexture(FUtuPluginTexture InUtuTexture) {
	// Format Paths
	TArray<FString> AssetNames = StartProcessAsset(InUtuTexture);
	// Invalid Asset
	if (DeleteInvalidAssetIfNeeded(AssetNames, UTexture2D::StaticClass())) {
		// Existing Asset
		UTexture2D* Asset = Cast<UTexture2D>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
		LogAssetImportOrReimport(Asset);
		// Create Asset
		AssetTools->Get().ImportAssetTasks({ BuildTask(InUtuTexture.texture_file_absolute_filename, AssetNames, nullptr) });
		Asset = Cast<UTexture2D>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
		if (Asset != nullptr) {
			Asset->PreEditChange(NULL);
			Asset->CompressionSettings = ImportSettings.CompressionSettings;
			Asset->Filter = ImportSettings.Filter;
			Asset->LODGroup = ImportSettings.LODGroup;
			Asset->SRGB = ImportSettings.SRGB;
			Asset->MaxTextureSize = ImportSettings.MaxTextureSize;
			Asset->CompressionQuality = ImportSettings.CompressionQuality;
			Asset->MipGenSettings = ImportSettings.MipGenSettings;
			Asset->DeferCompression = false;
			Asset->PostEditChange();
		}
		LogAssetImportedOrFailed(Asset, AssetNames, InUtuTexture.texture_file_absolute_filename, "Texture", { "Invalid Texture File : Make sure that the texture file is a supported format by trying to import it manually in Unreal." });
	}
}

void FUtuPluginAssetTypeProcessor::LogAssetCreateOrNot(UObject* InAsset) {
	if (InAsset == nullptr) {
		UTU_LOG_L("    New Asset. Creating...");
	}
	else {
		UTU_LOG_L("    Existing Asset.");
	}
}

void FUtuPluginAssetTypeProcessor::LogAssetImportOrReimport(UObject* InAsset) {
	if (InAsset == nullptr) {
		UTU_LOG_L("    New Asset. Importing..."); 
	}
	else { 
		UTU_LOG_L("    Existing Asset. Re-Importing..."); 
	}
}

void FUtuPluginAssetTypeProcessor::LogAssetImportedOrFailed(UObject* InAsset, TArray<FString> InAssetNames, FString InSourceFileFullname, FString InAssetType, TArray<FString> InPotentialCauses) {
	if (InAsset != nullptr) {
		FAssetRegistryModule::AssetCreated(InAsset);
		UTU_LOG_L("            Asset Created.");
	}
	else {
		UTU_LOG_E("    Asset Name: " + InAssetNames[1]);
		UTU_LOG_E("        Unreal Asset Relative Filename: " + InAssetNames[2]);
		if (InSourceFileFullname != "") {
			UTU_LOG_E("        Source File Fullname: " + InSourceFileFullname);
		}
		UTU_LOG_E("            Failed to create new " + InAssetType);
		UTU_LOG_E("            Potential Causes:");
		if (InPotentialCauses.Num() == 0) {
			InPotentialCauses.Add("No Potential Causes known yet.");
		}
		for (FString x : InPotentialCauses) {
			UTU_LOG_E("                - " + x);
		}
		UTU_LOG_E("            Asset skipped.");
	}
}


void FUtuPluginAssetTypeProcessor::ProcessPrefabFirstPass(FUtuPluginPrefabFirstPass InUtuPrefabFirstPass) {
	// Make sure it does not save the bp on compile
	UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
	ESaveOnCompile OriginalSaveOnCompile = Settings->SaveOnCompile;
	Settings->SaveOnCompile = ESaveOnCompile::SoC_Never;
	Settings->SaveConfig();
	// Format Paths
	TArray<FString> AssetNames = StartProcessAsset(InUtuPrefabFirstPass);
	// Invalid Asset
	if (DeleteInvalidAssetIfNeeded(AssetNames, UBlueprint::StaticClass())) {
		// Existing Asset
		UBlueprint* Asset = Cast<UBlueprint>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
		LogAssetCreateOrNot(Asset);
		// Create Asset
		IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
		if (Asset == nullptr) {
			// Create Bp
			UPackage* Package = CreateAssetPackage(AssetNames[2], false);
			UClass* BlueprintClass = nullptr;
			UClass* BlueprintGeneratedClass = nullptr;
			KismetCompilerModule.GetBlueprintTypesForClass(AActor::StaticClass(), BlueprintClass, BlueprintGeneratedClass);
			Asset = FKismetEditorUtilities::CreateBlueprint(AActor::StaticClass(), Package, *AssetNames[1], BPTYPE_Normal, BlueprintClass, BlueprintGeneratedClass, FName("LevelEditorActions"));
			LogAssetImportedOrFailed(Asset, AssetNames, "", "Blueprint", { });
			// Create Root Node
			BpAddRootComponent(Asset, InUtuPrefabFirstPass.has_any_static_child);
		}
		if (Asset != nullptr) {
			Asset->PreEditChange(NULL);
			// Delete Existing Nodes -- TODO : Analyse them and keep the good ones
			TArray<USCS_Node*> ExistingNodes = Asset->SimpleConstructionScript->GetAllNodes();
			if (ExistingNodes.Num() > 1) {
				UTU_LOG_L("    Detected existing Nodes. Deleting them and re-building the Blueprint from scratch.");
				//UTU_LOG_L("        Plan for a future release: Analyse existing Nodes and keep the good ones.");
				//for (USCS_Node* Node : ExistingNodes) {
				//	Node->EditorComponentInstance->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));
				//}
				while (ExistingNodes.Num() > 1) {
					TArray<USCS_Node*> RootNodes = Asset->SimpleConstructionScript->GetRootNodes();
					for (USCS_Node* Node : ExistingNodes) {
						if (!RootNodes.Contains(Node)) {
							Asset->SimpleConstructionScript->RemoveNode(Node);
						}
					}
					Asset->SimpleConstructionScript->FixupRootNodeParentReferences();
					ExistingNodes = Asset->SimpleConstructionScript->GetAllNodes();
				}
			}
			Cast<USceneComponent>(ExistingNodes[0]->ComponentTemplate)->SetMobility(InUtuPrefabFirstPass.has_any_static_child ? EComponentMobility::Static : EComponentMobility::Movable);
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Asset);
			FKismetEditorUtilities::CompileBlueprint(Asset);
			Asset->PostEditChange();
		}
	}
	// Restore Save On Compile
	Settings->SaveOnCompile = OriginalSaveOnCompile;
	Settings->SaveConfig();
}

void FUtuPluginAssetTypeProcessor::ProcessPrefabSecondPass(FUtuPluginPrefabSecondPass InUtuPrefabSecondPass) {
	// Make sure it does not save the bp on compile
	UBlueprintEditorSettings* Settings = GetMutableDefault<UBlueprintEditorSettings>();
	ESaveOnCompile OriginalSaveOnCompile = Settings->SaveOnCompile;
	Settings->SaveOnCompile = ESaveOnCompile::SoC_Never;
	Settings->SaveConfig();
	// Format Paths
	TArray<FString> AssetNames = StartProcessAsset(InUtuPrefabSecondPass);
	// Existing Asset
	UBlueprint* Asset = Cast<UBlueprint>(UUtuPluginLibrary::TryGetAsset(AssetNames[2]));
	// Skipping Asset. Should already be created by FirstPass
	if (Asset == nullptr) {
		UTU_LOG_W("    Asset Name: " + AssetNames[1]);
		UTU_LOG_W("        Unreal Asset Relative Path: " + AssetNames[2]);
		UTU_LOG_W("            Cannot setup Blueprint because the asset does not exist.");
		UTU_LOG_W("            Potential Causes:");
		UTU_LOG_W("                - PrefabFirstPass failed to create the asset.");
		UTU_LOG_W("            Asset skipped.");
		return;
	}
	Asset->PreEditChange(NULL);
	// Process Asset
	// Maps
	TMap<int, USCS_Node*> IdToNode;
	TMap<USCS_Node*, int> NodeToParentId;
	// Get Root Node
	int RootId = UtuConst::INVALID_INT;
	USCS_Node* RootNode = Asset->SimpleConstructionScript->GetRootNodes()[0];
	IdToNode.Add(RootId, RootNode);
	// Add Real Components
	TArray<FString> UniqueNames;
	UTU_LOG_L("    Adding real components...");
	for (FUtuPluginActor PrefabComponent : InUtuPrefabSecondPass.prefab_components) {
		// New Component
		FString ComponentName = BpMakeUniqueName(PrefabComponent.actor_display_name, UniqueNames);
		USCS_Node* ComponentNode = nullptr;
		bool bComponentCreated = BpAddRootComponentForSubComponentsIfNeeded(Asset, PrefabComponent, ComponentName, ComponentNode);
		if (bComponentCreated) {
			IdToNode.Add(PrefabComponent.actor_id, ComponentNode);
			NodeToParentId.Add(ComponentNode, PrefabComponent.actor_parent_id);
		}
		// Real Components for real this time
		for (EUtuActorType CompType : PrefabComponent.actor_types) {
			USCS_Node* SubComponentNode = nullptr;
			FString SubComponentName = bComponentCreated ? BpMakeUniqueName(PrefabComponent.actor_display_name, UniqueNames) : ComponentName;
			if (CompType == EUtuActorType::Empty) {
				BpAddEmptyComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::StaticMesh) {
				BpAddStaticMeshComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::SkeletalMesh) {
				BpAddSkeletalMeshComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);  // TODO : Support Skeletal Mesh
				//BpAddStaticMeshComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::PointLight) {
				BpAddPointLightComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::DirectionalLight) {
				BpAddDirectionalLightComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::SpotLight) {
				BpAddSpotLightComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::Camera) {
				BpAddCameraComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			else if (CompType == EUtuActorType::Prefab) {
				BpAddChildActorComponent(Asset, PrefabComponent, SubComponentName, SubComponentNode, bComponentCreated);
			}
			// Attachment
			if (bComponentCreated) {
				ComponentNode->AddChildNode(SubComponentNode);
			}
			else {
				IdToNode.Add(PrefabComponent.actor_id, SubComponentNode);
				NodeToParentId.Add(SubComponentNode, PrefabComponent.actor_parent_id);
			}
		}
	}
	// Parent Nodes
	if (Asset != nullptr) {
		UTU_LOG_L("        Parenting components...");
		TArray<USCS_Node*> Keys;
		NodeToParentId.GetKeys(Keys);
		for (USCS_Node* Node : Keys) {
			int Id = NodeToParentId[Node];
			USCS_Node* ParentNode = IdToNode[Id];
			ParentNode->AddChildNode(Node);
			UTU_LOG_L("            " + Node->GetName() + " -> " + ParentNode->GetName());
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Asset);
		FKismetEditorUtilities::CompileBlueprint(Asset);
		Asset->MarkPackageDirty();
		Asset->PostEditChange();
	}
	// Restore Save On Compile
	Settings->SaveOnCompile = OriginalSaveOnCompile;
	Settings->SaveConfig();
	Settings->RemoveFromRoot();
}

void FUtuPluginAssetTypeProcessor::BpAddRootComponent(UBlueprint* InAsset, bool bStatic) {
	int RootNodeId = UtuConst::INVALID_INT;
	UTU_LOG_L("    Adding Root component...");
	UTU_LOG_L("        Component Name: 'Root'");
	UTU_LOG_L("        Component ID: " + FString::FromInt(RootNodeId));
	UTU_LOG_L("        Component Class: 'USceneComponent'");
	USceneComponent* Root = NewObject<USceneComponent>(InAsset, TEXT("Root"));
	Root->SetMobility(bStatic ? EComponentMobility::Static : EComponentMobility::Movable);
	Root->ComponentTags.Add(*FString::FromInt(RootNodeId));
	USCS_Node* RootNode = InAsset->SimpleConstructionScript->CreateNode(Root->GetClass(), Root->GetFName());
	//InAsset->SimpleConstructionScript->RemoveNode(InAsset->SimpleConstructionScript->GetRootNodes()[0]);
	InAsset->SimpleConstructionScript->AddNode(RootNode); // Making it the new root
	//USCS_Node* RootNode = InAsset->SimpleConstructionScript->GetRootNodes()[0];
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Root, RootNode->ComponentTemplate);
	//FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(InAsset);
	// TODO : Handle Root Node! Can't Remove it because it's the root
	//OutRootNode = InAsset->SimpleConstructionScript->CreateNode(OutRoot->GetClass(), OutRoot->GetFName());
	//UEditorEngine::CopyPropertiesForUnrelatedObjects(OutRoot, OutRootNode->ComponentTemplate);
	//InAsset->SimpleConstructionScript->AddNode(RootNode); // Making it the new root
	//InAsset->SimpleConstructionScript->FixupRootNodeParentReferences();
}



AActor* FUtuPluginAssetTypeProcessor::WorldAddRootActorForSubActorsIfNeeded(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	AActor* RetActor = nullptr;
	UTU_LOG_L("        Processing '" + InUtuActor.actor_display_name + "'...");
	if (InUtuActor.actor_types.Num() != 1 || InUtuActor.actor_types[0] == EUtuActorType::Empty) { // If actor_types == 1, we don't need to have an empty root above the other actors.
		UTU_LOG_L("            Because there was more than one supported components on this GameObject in Unity, creating another 'Root' to hold them...");
		UTU_LOG_L("                Adding Root actor...");
		UTU_LOG_L("                    Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_L("                    Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_L("                    Actor Class: 'AActor'");
		UTU_LOG_L("                    Actor Tag: '" + InUtuActor.actor_tag + "'");
		// Create Empty Root Actor
		FActorSpawnParameters Params = FActorSpawnParameters();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		RetActor = InAsset->SpawnActor<AActor>(Params);
		if (RetActor != nullptr) {
			USceneComponent* RootComponent = NewObject<USceneComponent>(RetActor, USceneComponent::GetDefaultSceneRootVariableName(), RF_Transactional);
			if (RootComponent != nullptr) {
				RetActor->SetRootComponent(RootComponent);
				RetActor->AddInstanceComponent(RootComponent);
				RootComponent->RegisterComponent();
				RetActor->SetActorLabel(InUtuActor.actor_display_name);
				RetActor->Tags.Add(*FString::FromInt(InUtuActor.actor_id));
				if (InUtuActor.actor_tag != "Untagged") {
					RetActor->Tags.Add(*InUtuActor.actor_tag);
				}
				RetActor->SetActorHiddenInGame(!InUtuActor.actor_is_visible);
				RetActor->SetActorLocation(UtuConst::ConvertLocation(InUtuActor.actor_world_location));
				RetActor->SetActorRotation(UtuConst::ConvertRotation(InUtuActor.actor_world_rotation));
				RetActor->SetActorScale3D(UtuConst::ConvertScale(InUtuActor.actor_world_scale));
				RetActor->GetRootComponent()->SetMobility(InUtuActor.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
			}
			else {
				UTU_LOG_E("            Failed to spawn Root Actor's Root Component...");
				UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
				UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
				UTU_LOG_E("                Potential Causes:");
				UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
				return nullptr; // Don't even bother returning the actor
			}
		}
		else {
			UTU_LOG_E("            Failed to spawn Root Actor...");
			UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
			UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
			UTU_LOG_E("                Potential Causes:");
			UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
		}
	}
	return RetActor;
}

AActor* FUtuPluginAssetTypeProcessor::WorldSpawnStaticMeshActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	UTU_LOG_L("            Adding Static Mesh Actor...");
	UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
	UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
	UTU_LOG_L("                Actor Class: 'AStaticMeshActor'");
	UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* RetActor = InAsset->SpawnActor<AStaticMeshActor>(Params);
	if (RetActor != nullptr) {
		TArray<FString> MeshNames = FormatRelativeFilenameForUnreal(InUtuActor.actor_mesh.actor_mesh_relative_filename);
		TArray<FString> MeshNamesSeparated = FormatRelativeFilenameForUnreal(InUtuActor.actor_mesh.actor_mesh_relative_filename_if_separated);
		UTU_LOG_L("            Associating Static Mesh to Static Mesh Actor...");
		UTU_LOG_L("                Unreal Asset Relative Path: " + MeshNames[2]);
		UTU_LOG_L("                Unreal Asset Relative Path If Separated: " + MeshNamesSeparated[2]);
		UStaticMesh* StaticMeshAsset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(MeshNames[2]));
		if (StaticMeshAsset == nullptr && ImportSettings.bImportSeparated) {
			// Try separated way
			StaticMeshAsset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(MeshNamesSeparated[2]));
		}
		if (StaticMeshAsset == nullptr) {
			UTU_LOG_W("                Failed to assign Static Mesh because it doesn't exists: '" + MeshNames[2] + "'");
		}
		RetActor->GetStaticMeshComponent()->SetStaticMesh(StaticMeshAsset);

		UTU_LOG_L("            Associating Materials to Static Mesh...");
		for (int x = 0; x < InUtuActor.actor_mesh.actor_mesh_materials_relative_filenames.Num(); x++) {
			TArray<FString> MatNames = FormatRelativeFilenameForUnreal(InUtuActor.actor_mesh.actor_mesh_materials_relative_filenames[x], true);
			UTU_LOG_L("                MaterialId[" + FString::FromInt(x) + "] : " + MatNames[2]);
			UMaterial* MaterialAsset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(MatNames[2]));
			RetActor->GetStaticMeshComponent()->SetMaterial(x, MaterialAsset);
			if (MaterialAsset == nullptr) {
				UTU_LOG_W("                    Failed to assign material because it doesn't exists: '" + MatNames[2] + "'");
			}
		}
		RetActor->SetActorLabel(InUtuActor.actor_display_name);
	}
	else {
		UTU_LOG_E("            Failed to spawn StaticMesh Actor...");
		UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_E("                Potential Causes:");
		UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
	}
	return RetActor;
}

AActor* FUtuPluginAssetTypeProcessor::WorldSpawnSkeletalMeshActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	UTU_LOG_L("            Adding Skeletal Mesh Actor...");
	UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
	UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
	UTU_LOG_L("                Actor Class: 'ASkeletalMeshActor'");
	UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASkeletalMeshActor* RetActor = InAsset->SpawnActor<ASkeletalMeshActor>(Params);
	if (RetActor != nullptr) {
		TArray<FString> MeshNames = FormatRelativeFilenameForUnreal(InUtuActor.actor_mesh.actor_mesh_relative_filename);
		UTU_LOG_L("            Associating Skeletal Mesh to Skeletal Mesh Actor...");
		UTU_LOG_L("                Unreal Asset Relative Path: " + MeshNames[2]);
		USkeletalMesh* SkeletalMeshAsset = Cast<USkeletalMesh>(UUtuPluginLibrary::TryGetAsset(MeshNames[2]));
		RetActor->GetSkeletalMeshComponent()->SetSkeletalMesh(SkeletalMeshAsset);
		if (SkeletalMeshAsset == nullptr) {
			UTU_LOG_W("                Failed to assign Skeletal Mesh because it doesn't exists: '" + MeshNames[2] + "'");
		}
		UTU_LOG_L("            Associating Materials to Skeletal Mesh...");
		for (int x = 0; x < InUtuActor.actor_mesh.actor_mesh_materials_relative_filenames.Num(); x++) {
			TArray<FString> MatNames = FormatRelativeFilenameForUnreal(InUtuActor.actor_mesh.actor_mesh_materials_relative_filenames[x], true);
			UTU_LOG_L("                MaterialId[" + FString::FromInt(x) + "] : " + MatNames[2]);
			UMaterial* MaterialAsset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(MatNames[2]));
			RetActor->GetSkeletalMeshComponent()->SetMaterial(x, MaterialAsset);
			if (MaterialAsset == nullptr) {
				UTU_LOG_W("                    Failed to assign material because it doesn't exists: '" + MatNames[2] + "'");
			}
		}
		RetActor->SetActorLabel(InUtuActor.actor_display_name);
	}
	else {
		UTU_LOG_E("            Failed to spawn StaticMesh Actor...");
		UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_E("                Potential Causes:");
		UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
	}
	return RetActor;
}

AActor* FUtuPluginAssetTypeProcessor::WorldSpawnBlueprintActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	TArray<FString> BpNames = FormatRelativeFilenameForUnreal(InUtuActor.actor_prefab.actor_prefab_relative_filename);
	UTU_LOG_L("            Adding Blueprint Actor...");
	UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
	UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
	UTU_LOG_L("                Actor Class: 'AActor'");
	UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
	UTU_LOG_L("                Unreal Asset Relative Path: " + BpNames[2]);
	AActor* RetActor = nullptr;
	UBlueprint* BlueprintAsset = Cast<UBlueprint>(UUtuPluginLibrary::TryGetAsset(BpNames[2]));
	if (BlueprintAsset != nullptr) {
		FActorSpawnParameters Params = FActorSpawnParameters();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		RetActor = InAsset->SpawnActor<AActor>(BlueprintAsset->GeneratedClass, Params);
		if (RetActor != nullptr) {
			RetActor->SetActorLabel(InUtuActor.actor_display_name);
		}
		else {
			UTU_LOG_E("            Failed to spawn Blueprint Actor...");
			UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
			UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
			UTU_LOG_E("                Unreal Asset Relative Path: " + BpNames[2]);
			UTU_LOG_E("                Potential Causes:");
			UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
		}
	}
	else {
		UTU_LOG_W("            Cannot spawn Blueprint Actor because Blueprint asset does not exist...");
		UTU_LOG_W("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_W("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_W("                Unreal Asset Relative Path: " + BpNames[2]);
	}
	return RetActor;
}

AActor* FUtuPluginAssetTypeProcessor::WorldSpawnSkyLightActor(UWorld* InAsset) {
	UTU_LOG_L("            Adding Point Sky Actor...");
	UTU_LOG_L("                Actor Name: 'SkyLight'");
	UTU_LOG_L("                Actor Class: 'ASkyLight'");
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASkyLight* RetActor = InAsset->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), Params);
	if (RetActor != nullptr && Cast<USkyLightComponent>(RetActor->GetLightComponent()) != nullptr) {
		USkyLightComponent* Comp = Cast<USkyLightComponent>(RetActor->GetLightComponent());
		RetActor->Tags.Add("UtuActor");
		RetActor->SetActorLabel("SkyLight");
		RetActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);
		Comp->Intensity = ImportSettings.SkyLightIntensity;
		Comp->SourceType = ESkyLightSourceType::SLS_SpecifiedCubemap;
		Comp->Cubemap = Cast<UTextureCube>(UUtuPluginLibrary::TryGetAsset("/UtuPlugin/Components/TX_CubeMap"));
	}
	else {
		UTU_LOG_E("            Failed to spawn Point Light Actor...");
		UTU_LOG_E("                Actor Name: 'SkyLight'");
		UTU_LOG_E("                Potential Causes:");
		UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
	}
	return RetActor;
}


AActor* FUtuPluginAssetTypeProcessor::WorldSpawnPointLightActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	UTU_LOG_L("            Adding Point Light Actor...");
	UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
	UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
	UTU_LOG_L("                Actor Class: 'APointLight'");
	UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APointLight* RetActor = InAsset->SpawnActor<APointLight>(APointLight::StaticClass(), Params);
	if (RetActor != nullptr && Cast<UPointLightComponent>(RetActor->GetLightComponent()) != nullptr) {
		UPointLightComponent* Comp = Cast<UPointLightComponent>(RetActor->GetLightComponent());
		RetActor->SetActorLabel(InUtuActor.actor_display_name);
		Comp->Intensity = InUtuActor.actor_light.light_intensity * ImportSettings.LightIntensityMultiplier;
		Comp->bUseInverseSquaredFalloff = false;
		Comp->SetLightFalloffExponent(ImportSettings.LightFalloffExponent);
		Comp->SetLightColor(HexToColor(InUtuActor.actor_light.light_color));
		Comp->AttenuationRadius = InUtuActor.actor_light.light_range * ImportSettings.LightRangeMultiplier;
		Comp->SetAttenuationRadius(InUtuActor.actor_light.light_range * ImportSettings.LightRangeMultiplier);
		Comp->CastShadows = InUtuActor.actor_light.light_is_casting_shadows;
	}
	else {
		UTU_LOG_E("            Failed to spawn Point Light Actor...");
		UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_E("                Potential Causes:");
		UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
	}
	return RetActor;
}


AActor* FUtuPluginAssetTypeProcessor::WorldSpawnDirectionalLightActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	UTU_LOG_L("            Adding Directional Light Actor...");
	UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
	UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
	UTU_LOG_L("                Actor Class: 'ADirectionalLight'");
	UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADirectionalLight* RetActor = InAsset->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Params);
	if (RetActor != nullptr && RetActor->GetLightComponent() != nullptr) {
		ULightComponent* Comp = RetActor->GetLightComponent();
		RetActor->SetActorLabel(InUtuActor.actor_display_name);
		Comp->Intensity = InUtuActor.actor_light.light_intensity * ImportSettings.LightIntensityMultiplier;
		Comp->SetLightColor(HexToColor(InUtuActor.actor_light.light_color));
		Comp->CastShadows = InUtuActor.actor_light.light_is_casting_shadows;
	}
	else {
		UTU_LOG_E("            Failed to spawn Directional Light Actor...");
		UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_E("                Potential Causes:");
		UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
	}
	return RetActor;
}

AActor* FUtuPluginAssetTypeProcessor::WorldSpawnSpotLightActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	UTU_LOG_L("            Adding Spot Light Actor...");
	UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
	UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
	UTU_LOG_L("                Actor Class: 'ASpotLight'");
	UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
	FActorSpawnParameters Params = FActorSpawnParameters();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASpotLight* RetActor = InAsset->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Params);
	if (RetActor != nullptr && Cast<USpotLightComponent>(RetActor->GetLightComponent()) != nullptr) {
		USpotLightComponent* Comp = Cast<USpotLightComponent>(RetActor->GetLightComponent());
		RetActor->SetActorLabel(InUtuActor.actor_display_name);
		Comp->Intensity = InUtuActor.actor_light.light_intensity * ImportSettings.LightIntensityMultiplier;
		Comp->bUseInverseSquaredFalloff = false;
		Comp->SetLightFalloffExponent(ImportSettings.LightFalloffExponent);
		Comp->SetLightColor(HexToColor(InUtuActor.actor_light.light_color));
		Comp->AttenuationRadius = InUtuActor.actor_light.light_range * ImportSettings.LightRangeMultiplier;
		Comp->SetAttenuationRadius(InUtuActor.actor_light.light_range * ImportSettings.LightRangeMultiplier);
		Comp->CastShadows = InUtuActor.actor_light.light_is_casting_shadows;
		Comp->InnerConeAngle = ImportSettings.LightSpotInnerConeAngle;
		Comp->OuterConeAngle = InUtuActor.actor_light.light_spot_angle * ImportSettings.LightSpotAngleMultiplier;
	}
	else {
		UTU_LOG_E("            Failed to spawn Spot Light Actor...");
		UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_E("                Potential Causes:");
		UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
	}
	return RetActor;
}

AActor* FUtuPluginAssetTypeProcessor::WorldSpawnCameraActor(UWorld* InAsset, FUtuPluginActor InUtuActor) {
	AActor* RetActor = nullptr;
	if (InUtuActor.actor_camera.camera_is_physical) {
		UTU_LOG_L("            Adding Cine Camera Actor...");
		UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_L("                Actor Class: 'ACineCameraActor'");
		UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
		FActorSpawnParameters Params = FActorSpawnParameters();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		RetActor = InAsset->SpawnActor<ACineCameraActor>(ACineCameraActor::StaticClass(), Params);
		if (RetActor != nullptr && Cast<ACineCameraActor>(RetActor)->GetCineCameraComponent() != nullptr) {
			UCineCameraComponent* Comp = Cast<ACineCameraActor>(RetActor)->GetCineCameraComponent();
			RetActor->SetActorLabel(InUtuActor.actor_display_name);
			//InUtuActor.actor_camera.camera_viewport_rect;
			Comp->OrthoNearClipPlane = InUtuActor.actor_camera.camera_near_clip_plane;
			Comp->OrthoFarClipPlane = InUtuActor.actor_camera.camera_far_clip_plane;
			Comp->AspectRatio = InUtuActor.actor_camera.camera_aspect_ratio;
			Comp->ProjectionMode = InUtuActor.actor_camera.camera_is_perspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic;
			Comp->OrthoWidth = InUtuActor.actor_camera.camera_ortho_size;
			Comp->FieldOfView = InUtuActor.actor_camera.camera_persp_field_of_view;
			Comp->CurrentFocalLength = InUtuActor.actor_camera.camera_phys_focal_length;
#if ENGINE_MINOR_VERSION >= 24 || ENGINE_MAJOR_VERSION >= 5
			Comp->Filmback.SensorWidth = InUtuActor.actor_camera.camera_phys_sensor_size.X;
			Comp->Filmback.SensorHeight = InUtuActor.actor_camera.camera_phys_sensor_size.Y;
			Comp->Filmback.SensorAspectRatio = InUtuActor.actor_camera.camera_aspect_ratio;
#else
			Comp->FilmbackImportSettings.SensorWidth = InUtuActor.actor_camera.camera_phys_sensor_size.X;
			Comp->FilmbackImportSettings.SensorHeight = InUtuActor.actor_camera.camera_phys_sensor_size.Y;
			Comp->FilmbackImportSettings.SensorAspectRatio = InUtuActor.actor_camera.camera_aspect_ratio;
#endif
		}
		else {
			UTU_LOG_E("            Failed to spawn Cine Camera Actor...");
			UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
			UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
			UTU_LOG_E("                Potential Causes:");
			UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
		}
	}
	else {
		UTU_LOG_L("            Adding Camera Actor...");
		UTU_LOG_L("                Actor Name: '" + InUtuActor.actor_display_name + "'");
		UTU_LOG_L("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
		UTU_LOG_L("                Actor Class: 'ACameraActor'");
		UTU_LOG_L("                Actor Tag: '" + InUtuActor.actor_tag + "'");
		FActorSpawnParameters Params = FActorSpawnParameters();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		RetActor = InAsset->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Params);
		if (RetActor != nullptr && Cast<ACameraActor>(RetActor)->GetCameraComponent() != nullptr) {
			UCameraComponent* Comp = Cast<ACameraActor>(RetActor)->GetCameraComponent();
			RetActor->SetActorLabel(InUtuActor.actor_display_name);
			//InUtuActor.actor_camera.camera_viewport_rect;
			Comp->OrthoNearClipPlane = InUtuActor.actor_camera.camera_near_clip_plane;
			Comp->OrthoFarClipPlane = InUtuActor.actor_camera.camera_far_clip_plane;
			Comp->AspectRatio = InUtuActor.actor_camera.camera_aspect_ratio;
			Comp->ProjectionMode = InUtuActor.actor_camera.camera_is_perspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic;
			Comp->OrthoWidth = InUtuActor.actor_camera.camera_ortho_size;
			Comp->FieldOfView = InUtuActor.actor_camera.camera_persp_field_of_view;
		}
		else {
			UTU_LOG_E("            Failed to spawn Camera Actor...");
			UTU_LOG_E("                Actor Name: '" + InUtuActor.actor_display_name + "'");
			UTU_LOG_E("                Actor ID: " + FString::FromInt(InUtuActor.actor_id));
			UTU_LOG_E("                Potential Causes:");
			UTU_LOG_E("                    - No Potential Causes known yet, but it should never happen.");
		}
	}
	return RetActor;
}

bool FUtuPluginAssetTypeProcessor::BpAddRootComponentForSubComponentsIfNeeded(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode) {
	OutComponentNode = nullptr;
	UTU_LOG_L("        Processing '" + InUniqueName + "'...");
	if (InPrefabComponent.actor_types.Num() != 1) { // If actor_types == 1, we don't need to have an empty root above the other components.
		UTU_LOG_L("            Because there was more than one supported components on this GameObject in Unity, creating another 'Root' to hold them...");
		UTU_LOG_L("                Adding Root component...");
		UTU_LOG_L("                    Component Name: '" + InUniqueName + "'");
		UTU_LOG_L("                    Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
		UTU_LOG_L("                    Component Class: 'USceneComponent'");
		UTU_LOG_L("                    Component Tag: '" + InPrefabComponent.actor_tag + "'");
		// Create Component
		USceneComponent*Component = NewObject<USceneComponent>(InAsset, *InUniqueName);
		Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
		// Create Component Node
		OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
		UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
		return true;
	}
	return false;
}

void FUtuPluginAssetTypeProcessor::BpAddEmptyComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	// If we get here, it's only because theres only one empty component to create. Should be almost the same as creating an intermediary root component
	UTU_LOG_L("            Adding Empty component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	UTU_LOG_L("                Component Class: 'USceneComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	USceneComponent* Component = NewObject<USceneComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

void FUtuPluginAssetTypeProcessor::BpAddStaticMeshComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	UTU_LOG_L("            Adding StaticMesh component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	if (!bInRootCreated) {
		UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	}
	UTU_LOG_L("                Component Class: 'UStaticMeshComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(ImportSettings.bImportSeparated ? InPrefabComponent.actor_relative_location_if_separated : InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Mesh Component Specific
	TArray<FString> MeshNames = FormatRelativeFilenameForUnreal(InPrefabComponent.actor_mesh.actor_mesh_relative_filename);
	TArray<FString> MeshNamesSeparated = FormatRelativeFilenameForUnreal(InPrefabComponent.actor_mesh.actor_mesh_relative_filename_if_separated);
	UStaticMesh* StaticMeshAsset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(MeshNames[2]));
	if (StaticMeshAsset == nullptr && ImportSettings.bImportSeparated)
	{
		// Try separated way
		StaticMeshAsset = Cast<UStaticMesh>(UUtuPluginLibrary::TryGetAsset(MeshNamesSeparated[2]));
	}
	Component->SetStaticMesh(StaticMeshAsset);
	if (StaticMeshAsset != nullptr) {
		for (int x = 0; x < InPrefabComponent.actor_mesh.actor_mesh_materials_relative_filenames.Num(); x++) {
			TArray<FString> MatNames = FormatRelativeFilenameForUnreal(InPrefabComponent.actor_mesh.actor_mesh_materials_relative_filenames[x], true);
			UMaterial* MaterialAsset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(MatNames[2]));
			Component->SetMaterial(x, MaterialAsset);
		}
	}
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

void FUtuPluginAssetTypeProcessor::BpAddSkeletalMeshComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	UTU_LOG_L("            Adding SkeletalMesh component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	if (!bInRootCreated) {
		UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	}
	UTU_LOG_L("                Component Class: 'USkeletalMeshComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		// Do not use the mesh transform since it's driven by the bones (Unity is dumb)
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Mesh Component Specific
	TArray<FString> MeshNames = FormatRelativeFilenameForUnreal(InPrefabComponent.actor_mesh.actor_mesh_relative_filename);
	USkeletalMesh* SkeletalMeshAsset = Cast<USkeletalMesh>(UUtuPluginLibrary::TryGetAsset(MeshNames[2]));
	Component->SetSkeletalMesh(SkeletalMeshAsset);
	if (SkeletalMeshAsset != nullptr) {
		for (int x = 0; x < InPrefabComponent.actor_mesh.actor_mesh_materials_relative_filenames.Num(); x++) {
			TArray<FString> MatNames = FormatRelativeFilenameForUnreal(InPrefabComponent.actor_mesh.actor_mesh_materials_relative_filenames[x], true);
			UMaterial* MaterialAsset = Cast<UMaterial>(UUtuPluginLibrary::TryGetAsset(MatNames[2]));
			Component->SetMaterial(x, MaterialAsset);
		}
	}
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

void FUtuPluginAssetTypeProcessor::BpAddPointLightComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	UTU_LOG_L("            Adding PointLight component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	if (!bInRootCreated) {
		UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	}
	UTU_LOG_L("                Component Class: 'UPointLightComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	UPointLightComponent* Component = NewObject<UPointLightComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Point Light Component Specific
	Component->Intensity = InPrefabComponent.actor_light.light_intensity * ImportSettings.LightIntensityMultiplier;
	Component->bUseInverseSquaredFalloff = false;
	Component->SetLightFalloffExponent(ImportSettings.LightFalloffExponent);
	Component->SetLightColor(HexToColor(InPrefabComponent.actor_light.light_color));
	Component->AttenuationRadius = InPrefabComponent.actor_light.light_range * ImportSettings.LightRangeMultiplier;
	Component->SetAttenuationRadius(InPrefabComponent.actor_light.light_range * ImportSettings.LightRangeMultiplier);
	Component->CastShadows = InPrefabComponent.actor_light.light_is_casting_shadows;
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

void FUtuPluginAssetTypeProcessor::BpAddDirectionalLightComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	UTU_LOG_L("            Adding DirectionalLight component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	if (!bInRootCreated) {
		UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	}
	UTU_LOG_L("                Component Class: 'UDirectionalLightComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	UDirectionalLightComponent* Component = NewObject<UDirectionalLightComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Point Light Component Specific
	Component->Intensity = InPrefabComponent.actor_light.light_intensity * ImportSettings.LightIntensityMultiplier;
	Component->SetLightColor(HexToColor(InPrefabComponent.actor_light.light_color));
	Component->CastShadows = InPrefabComponent.actor_light.light_is_casting_shadows;
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

void FUtuPluginAssetTypeProcessor::BpAddSpotLightComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	UTU_LOG_L("            Adding SpotLight component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	if (!bInRootCreated) {
		UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	}
	UTU_LOG_L("                Component Class: 'USpotLightComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	USpotLightComponent* Component = NewObject<USpotLightComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Point Light Component Specific
	Component->Intensity = InPrefabComponent.actor_light.light_intensity * ImportSettings.LightIntensityMultiplier;
	Component->bUseInverseSquaredFalloff = false;
	Component->SetLightFalloffExponent(ImportSettings.LightFalloffExponent);
	Component->SetLightColor(HexToColor(InPrefabComponent.actor_light.light_color));
	Component->AttenuationRadius = InPrefabComponent.actor_light.light_range * ImportSettings.LightRangeMultiplier;
	Component->SetAttenuationRadius(InPrefabComponent.actor_light.light_range * ImportSettings.LightRangeMultiplier);
	Component->CastShadows = InPrefabComponent.actor_light.light_is_casting_shadows;
	Component->InnerConeAngle = ImportSettings.LightSpotInnerConeAngle;
	Component->OuterConeAngle = InPrefabComponent.actor_light.light_spot_angle * ImportSettings.LightSpotAngleMultiplier;
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

void FUtuPluginAssetTypeProcessor::BpAddCameraComponent(UBlueprint* InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	if (InPrefabComponent.actor_camera.camera_is_physical) {
		UTU_LOG_L("            Adding Cine Camera Component...");
		UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
		if (!bInRootCreated) {
			UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
		}
		UTU_LOG_L("                Component Class: 'UCineCameraComponent'");
		UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
		// Create Component
		UCineCameraComponent* Component = NewObject<UCineCameraComponent>(InAsset, *InUniqueName);
		Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
		if (!bInRootCreated) {
			Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
			if (InPrefabComponent.actor_tag != "Untagged") {
				Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
			}
			Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
			Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
			Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
			Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
		}
		// Cine Camera Component Specific
		//InPrefabComponent.actor_camera.camera_viewport_rect;
		Component->OrthoNearClipPlane = InPrefabComponent.actor_camera.camera_near_clip_plane;
		Component->OrthoFarClipPlane = InPrefabComponent.actor_camera.camera_far_clip_plane;
		Component->AspectRatio = InPrefabComponent.actor_camera.camera_aspect_ratio;
		Component->ProjectionMode = InPrefabComponent.actor_camera.camera_is_perspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic;
		Component->OrthoWidth = InPrefabComponent.actor_camera.camera_ortho_size;
		Component->FieldOfView = InPrefabComponent.actor_camera.camera_persp_field_of_view;
		Component->CurrentFocalLength = InPrefabComponent.actor_camera.camera_phys_focal_length;
#if ENGINE_MINOR_VERSION >= 24 || ENGINE_MAJOR_VERSION >= 5
		Component->Filmback.SensorWidth = InPrefabComponent.actor_camera.camera_phys_sensor_size.X;
		Component->Filmback.SensorHeight = InPrefabComponent.actor_camera.camera_phys_sensor_size.Y;
		Component->Filmback.SensorAspectRatio = InPrefabComponent.actor_camera.camera_aspect_ratio;
#else
		Component->FilmbackImportSettings.SensorWidth = InPrefabComponent.actor_camera.camera_phys_sensor_size.X;
		Component->FilmbackImportSettings.SensorHeight = InPrefabComponent.actor_camera.camera_phys_sensor_size.Y;
		Component->FilmbackImportSettings.SensorAspectRatio = InPrefabComponent.actor_camera.camera_aspect_ratio;
#endif
		// Create Component Node
		OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
		UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
	}
	else {
		UTU_LOG_L("            Adding Camera Actor...");
		UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
		if (!bInRootCreated) {
			UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
		}
		UTU_LOG_L("                Component Class: 'UCameraComponent'");
		UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
		// Create Component
		UCameraComponent* Component = NewObject<UCameraComponent>(InAsset, *InUniqueName);
		Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
		if (!bInRootCreated) {
			Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
			if (InPrefabComponent.actor_tag != "Untagged") {
				Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
			}
			Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
			Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
			Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
			Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
		}
		// Camera Component Specific
		//InPrefabComponent.actor_camera.camera_viewport_rect;
		Component->OrthoNearClipPlane = InPrefabComponent.actor_camera.camera_near_clip_plane;
		Component->OrthoFarClipPlane = InPrefabComponent.actor_camera.camera_far_clip_plane;
		Component->AspectRatio = InPrefabComponent.actor_camera.camera_aspect_ratio;
		Component->ProjectionMode = InPrefabComponent.actor_camera.camera_is_perspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic;
		Component->OrthoWidth = InPrefabComponent.actor_camera.camera_ortho_size;
		Component->FieldOfView = InPrefabComponent.actor_camera.camera_persp_field_of_view;
		// Create Component Node
		OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
		UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
	}
}

void FUtuPluginAssetTypeProcessor::BpAddChildActorComponent(UBlueprint * InAsset, FUtuPluginActor InPrefabComponent, FString InUniqueName, USCS_Node*& OutComponentNode, bool bInRootCreated) {
	UTU_LOG_L("            Adding Prefab component...");
	UTU_LOG_L("                Component Name: '" + InUniqueName + "'");
	if (!bInRootCreated) {
		UTU_LOG_L("                Component ID: " + FString::FromInt(InPrefabComponent.actor_id));
	}
	UTU_LOG_L("                Component Class: 'UChildActorComponent'");
	UTU_LOG_L("                Component Tag: '" + InPrefabComponent.actor_tag + "'");
	// Create Component
	UChildActorComponent* Component = NewObject<UChildActorComponent>(InAsset, *InUniqueName);
	Component->SetMobility(InPrefabComponent.actor_is_movable ? EComponentMobility::Movable : EComponentMobility::Static);
	if (!bInRootCreated) {
		Component->ComponentTags.Add(*FString::FromInt(InPrefabComponent.actor_id));
		if (InPrefabComponent.actor_tag != "Untagged") {
			Component->ComponentTags.Add(*InPrefabComponent.actor_tag);
		}
		Component->SetHiddenInGame(!InPrefabComponent.actor_is_visible);
		Component->SetRelativeLocation(UtuConst::ConvertLocation(InPrefabComponent.actor_relative_location));
		Component->SetRelativeRotation(UtuConst::ConvertRotation(InPrefabComponent.actor_relative_rotation));
		Component->SetRelativeScale3D(UtuConst::ConvertScale(InPrefabComponent.actor_relative_scale));
	}
	// Child Actor Specific
	TArray<FString> BpNames = FormatRelativeFilenameForUnreal(InPrefabComponent.actor_prefab.actor_prefab_relative_filename);
	UBlueprint* BpChild = Cast<UBlueprint>(UUtuPluginLibrary::TryGetAsset(BpNames[2]));
	Component->SetChildActorClass(TSubclassOf<AActor>(BpChild->GeneratedClass));
	// Create Component Node
	OutComponentNode = InAsset->SimpleConstructionScript->CreateNode(Component->GetClass(), *InUniqueName);
	UEditorEngine::CopyPropertiesForUnrelatedObjects(Component, OutComponentNode->ComponentTemplate);
}

FString FUtuPluginAssetTypeProcessor::BpMakeUniqueName(FString InDesiredName, TArray<FString>& InOutUsedNames) {
	if (InOutUsedNames.Contains(InDesiredName)) {
		for (int x = 1; x < 1000; x++) {
			FString PotentialName = InDesiredName + "_" + FString::FromInt(x);
			if (!InOutUsedNames.Contains(PotentialName)) {
				InOutUsedNames.Add(PotentialName);
				return PotentialName;
			}
		}
	}
	InOutUsedNames.Add(InDesiredName);
	return InDesiredName;
}


UAssetImportTask* FUtuPluginAssetTypeProcessor::BuildTask(FString InSource, TArray<FString> InAssetNames, UObject* InOptions) {
	UAssetImportTask* RetTask = NewObject<UAssetImportTask>();
	RetTask->Filename = InSource;
	RetTask->DestinationPath = InAssetNames[0];
	RetTask->DestinationName = InAssetNames[1];
	RetTask->bSave = false;
	RetTask->bAutomated = true;
	RetTask->bReplaceExisting = true;
	RetTask->Options = InOptions;
	return RetTask;
}

TArray<FString> FUtuPluginAssetTypeProcessor::StartProcessAsset(FUtuPluginAsset InUtuAsset, bool bInIsMaterial) {
	TArray<FString> RetAssetNames = FormatRelativeFilenameForUnreal(InUtuAsset.asset_relative_filename, bInIsMaterial);
	UTU_LOG_EMPTY_LINE();
	UTU_LOG_L("Asset Name: " + RetAssetNames[1]);
	UTU_LOG_L("    Unity  Asset Relative Path: " + InUtuAsset.asset_relative_filename);
	UTU_LOG_L("    Unreal Asset Relative Path: " + RetAssetNames[2]);
	UTU_LOG_L("Begin Creation ...");
	UTU_LOG_L("    Time: " + FDateTime::UtcNow().ToString());
	return RetAssetNames;
}

bool FUtuPluginAssetTypeProcessor::DeleteInvalidAssetIfNeeded(TArray<FString> InAssetNames, UClass* InClass) {
	UObject* Asset = UUtuPluginLibrary::TryGetAsset(InAssetNames[2]);
	if (Asset != nullptr) {
		if (Asset->GetClass() != InClass) {
			if (bDeleteInvalidAssets) {
				UTU_LOG_L("    Existing Invalid Asset detected, deleting ...");
				if (UUtuPluginLibrary::DeleteAsset(Asset)) {
					UTU_LOG_L("        Invalid Asset deleted.");
				}
				else {
					UTU_LOG_E("    Asset Name: " + InAssetNames[1]);
					UTU_LOG_E("        Unreal Asset Relative Path: " + InAssetNames[2]);
					UTU_LOG_E("            You are trying to import a '" + InClass->GetDisplayNameText().ToString() + "' over an already existing asset that is of type '" + Asset->GetClass()->GetDisplayNameText().ToString() + "'.");
					UTU_LOG_E("            Tried to delete the Invalid Asset, but failed to delete it.");
					UTU_LOG_E("            Potential Causes:");
					UTU_LOG_E("                - The asset might be referenced by other content. Please delete it manually.");
					UTU_LOG_E("            Asset skipped.");
					return false;
				}
			}
			else {
				UTU_LOG_W("    Asset Name: " + InAssetNames[1]);
				UTU_LOG_W("        Unreal Asset Relative Path: " + InAssetNames[2]);
				UTU_LOG_W("            You are trying to import a '" + InClass->GetDisplayNameText().ToString() + "' over an already existing asset that is of type '" + Asset->GetClass()->GetDisplayNameText().ToString() + "'.");
				UTU_LOG_W("            If you want to process this asset, please delete the current asset or enable the 'Delete Invalid Assets' functionnality.");
				UTU_LOG_W("            Asset skipped.");
				return false;
			}
		}
	}
	return true;
}
