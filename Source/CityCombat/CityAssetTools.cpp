#include "CityAssetTools.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"
#if WITH_EDITOR
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "StaticMeshAttributes.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "Builders/CubeBuilder.h"
#include "Model.h"
#include "Components/BrushComponent.h"
#endif

UStaticMesh* UCityAssetTools::BuildRifleMesh()
{
#if WITH_EDITOR
    const TCHAR* Path = TEXT("/Game/SoulCityGame/Weapons/SM_MurdockPrimaryRifle.SM_MurdockPrimaryRifle");
    if (UStaticMesh* Existing = LoadObject<UStaticMesh>(nullptr, Path)) return Existing;
    USkeletalMesh* Source = LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Meshes/Murdock.Murdock"));
    if (!Source || !Source->GetImportedModel() || Source->GetImportedModel()->LODModels.Num()==0) return nullptr;
    const FSkeletalMeshLODModel& LOD = Source->GetImportedModel()->LODModels[0];
    const auto& Skeleton = Source->GetRefSkeleton();
    FMeshDescription Description;
    FStaticMeshAttributes Attributes(Description); Attributes.Register();
    FMeshDescriptionBuilder Builder; Builder.SetMeshDescription(&Description); Builder.EnablePolyGroups(); Builder.SetNumUVLayers(1);
    TArray<FSoftSkinVertex> Vertices; LOD.GetVertices(Vertices);
    TArray<FVertexID> IDs; IDs.SetNum(Vertices.Num());
    FBox Bounds(ForceInit);
    TArray<uint32> ChosenIndices;
    TArray<int32> ChosenMaterials;
    for (const FSkelMeshSection& Section : LOD.Sections)
    {
        const int32 MaterialIndex=Section.MaterialIndex;
        const auto& Material=Source->GetMaterials()[MaterialIndex];
        const FString MaterialName=Material.MaterialInterface ? Material.MaterialInterface->GetName() : Material.MaterialSlotName.ToString();
        if (!MaterialName.Contains(TEXT("Weapon"),ESearchCase::IgnoreCase)) continue;
        for (uint32 T=0; T<Section.NumTriangles; ++T)
        {
            uint32 Triangle[3]; bool Shotgun=false;
            for (int32 K=0;K<3;++K)
            {
                Triangle[K]=LOD.IndexBuffer[Section.BaseIndex+T*3+K];
                const FSoftSkinVertex& V=Vertices[Triangle[K]];
                int32 Strongest=0;
                for (int32 J=1;J<MAX_TOTAL_INFLUENCES;++J) if (V.InfluenceWeights[J]>V.InfluenceWeights[Strongest]) Strongest=J;
                const int32 Bone=Section.BoneMap[V.InfluenceBones[Strongest]];
                if (Skeleton.GetBoneName(Bone).ToString().StartsWith(TEXT("shotgun"))) Shotgun=true;
            }
            if (!Shotgun) continue;
            for (uint32 I:Triangle) { ChosenIndices.Add(I); Bounds+=FVector(Vertices[I].Position); }
            ChosenMaterials.Add(MaterialIndex);
        }
    }
    if (ChosenIndices.Num()<30) return nullptr;
    const FVector Center=Bounds.GetCenter();
    TMap<uint32,FVertexID> VertexMap;
    TMap<int32,FPolygonGroupID> Groups;
    for (int32 T=0;T<ChosenMaterials.Num();++T)
    {
        const int32 MaterialIndex=ChosenMaterials[T];
        if (!Groups.Contains(MaterialIndex)) Groups.Add(MaterialIndex,Builder.AppendPolygonGroup());
        FVertexInstanceID Instances[3];
        for (int32 K=0;K<3;++K)
        {
            const uint32 I=ChosenIndices[T*3+K]; const FSoftSkinVertex& V=Vertices[I];
            if (!VertexMap.Contains(I)) VertexMap.Add(I,Builder.AppendVertex(FVector(V.Position)-Center));
            Instances[K]=Builder.AppendInstance(VertexMap[I]);
            Builder.SetInstanceNormal(Instances[K],FVector(V.TangentZ));
            Builder.SetInstanceTangentSpace(Instances[K],FVector(V.TangentZ),FVector(V.TangentX),1.f);
            Builder.SetInstanceUV(Instances[K],FVector2D(V.UVs[0]),0);
            Builder.SetInstanceColor(Instances[K],FVector4f(1,1,1,1));
        }
        Builder.AppendTriangle(Instances[0],Instances[1],Instances[2],Groups[MaterialIndex]);
    }
    UPackage* Package=CreatePackage(TEXT("/Game/SoulCityGame/Weapons/SM_MurdockPrimaryRifle"));
    UStaticMesh* Result=NewObject<UStaticMesh>(Package,TEXT("SM_MurdockPrimaryRifle"),RF_Public|RF_Standalone);
    auto SlotNames=Attributes.GetPolygonGroupMaterialSlotNames();
    for (const auto& Pair:Groups)
    {
        const FSkeletalMaterial& Material=Source->GetMaterials()[Pair.Key];
        Result->GetStaticMaterials().Add(FStaticMaterial(Material.MaterialInterface,Material.MaterialSlotName));
        SlotNames[Pair.Value]=Material.MaterialSlotName;
    }
    UStaticMesh::FBuildMeshDescriptionsParams Params; Params.bBuildSimpleCollision=true; Params.bFastBuild=false;
    Result->BuildFromMeshDescriptions({&Description},Params);
    FAssetRegistryModule::AssetCreated(Result); Result->MarkPackageDirty();
    UE_LOG(LogTemp,Display,TEXT("CITY_RIFLE_EXTRACTED triangles=%d bounds=%s"),ChosenMaterials.Num(),*Bounds.GetSize().ToString());
    return Result;
#else
    return nullptr;
#endif
}

bool UCityAssetTools::BuildArenaNavigation(UWorld* World, FVector Center, FVector Size)
{
#if WITH_EDITOR
    if (!World) return false;
    ANavMeshBoundsVolume* Bounds=World->SpawnActor<ANavMeshBoundsVolume>(Center,FRotator::ZeroRotator);
    if (!Bounds) return false;
    Bounds->SetActorLabel(TEXT("Arena_Navigation"));
    Bounds->Brush=NewObject<UModel>(Bounds,NAME_None,RF_Transactional);
    Bounds->Brush->Initialize(nullptr,true);
    Bounds->GetBrushComponent()->Brush=Bounds->Brush;
    UCubeBuilder* Builder=NewObject<UCubeBuilder>(Bounds);
    Builder->X=Size.X; Builder->Y=Size.Y; Builder->Z=Size.Z;
    Builder->Build(World,Bounds);
    Bounds->GetBrushComponent()->BuildSimpleBrushCollision();
    Bounds->PostEditChange();
    if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
    {
        Nav->RemoveNavigationBuildLock(0xff,UNavigationSystemV1::ELockRemovalRebuildAction::NoRebuild);
        Nav->OnNavigationBoundsUpdated(Bounds);
        Nav->Build();
        if (auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::Create)) Data->EnsureBuildCompletion();
        return true;
    }
#endif
    return false;
}
