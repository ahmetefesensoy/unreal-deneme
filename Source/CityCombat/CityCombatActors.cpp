#include "CityCombatActors.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Particles/ParticleSystem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/TargetPoint.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "NavigationData.h"
#include "Misc/App.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ACityCharacter::ACityCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);
    GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    GetMesh()->SetRelativeLocation(FVector(0, 0, -88));
    GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    bUseControllerRotationYaw = true;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = false;
    Movement->MaxWalkSpeed = WalkSpeed;
    Movement->MaxAcceleration = 1800.f;
    Movement->BrakingDecelerationWalking = 2400.f;
    Movement->GroundFriction = 8.f;
    Movement->MaxStepHeight = 55.f;
    Movement->SetWalkableFloorAngle(48.f);
    Movement->bUseFlatBaseForFloorChecks = true;
    Movement->PerchRadiusThreshold = 10.f;
    Movement->JumpZVelocity = 420.f;
    JumpMaxCount = 1;
    Movement->AirControl = 0.25f;
    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 320.f;
    CameraBoom->SocketOffset = FVector(0, 70, 70);
    CameraBoom->bUsePawnControlRotation = true;
    CameraBoom->ProbeSize = 10.f;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    Camera->SetupAttachment(CameraBoom);
    Camera->FieldOfView = 90.f;
    OpticRoot=CreateDefaultSubobject<USceneComponent>(TEXT("RifleOptic"));
    OpticRoot->SetupAttachment(GetMesh(),TEXT("gun_scope"));
    OpticRoot->SetRelativeLocation(FVector(0,0,7));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> OpticCylinder(TEXT("/Engine/BasicShapes/Cylinder"));
    auto* Housing=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SoulCityGame/Weapons/Materials/M_OpticHousing.M_OpticHousing"));
    auto* Glass=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/SoulCityGame/Weapons/Materials/M_OpticGlass.M_OpticGlass"));
    auto OpticPart=[&](const TCHAR* Name,FVector Location,FVector Scale,bool Lens) {
        auto* Part=CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Part->SetupAttachment(OpticRoot); Part->SetStaticMesh(OpticCylinder.Object);
        Part->SetRelativeLocation(Location); Part->SetRelativeRotation(FRotator(90,0,0));
        Part->SetRelativeScale3D(Scale); Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCastShadow(!Lens);
        if (auto* Material=Lens?Glass:Housing) Part->SetMaterial(0,Material);
    };
    OpticPart(TEXT("OpticTube"),FVector::ZeroVector,FVector(0.11f,0.11f,0.24f),false);
    OpticPart(TEXT("OpticFrontRing"),FVector(13,0,0),FVector(0.16f,0.16f,0.045f),false);
    OpticPart(TEXT("OpticRearRing"),FVector(-13,0,0),FVector(0.13f,0.13f,0.045f),false);
    OpticPart(TEXT("OpticLens"),FVector(15.4f,0,0),FVector(0.125f,0.125f,0.008f),true);
    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CharacterFill"));
    FillLight->SetupAttachment(RootComponent);
    FillLight->SetRelativeLocation(FVector(80, 0, 150));
    FillLight->Intensity = 8.f;
    FillLight->AttenuationRadius = 180.f;
    FillLight->SetLightColor(FLinearColor(0.55f, 0.7f, 1.f));
    FillLight->SetCastShadows(false);
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Meshes/Murdock"));
    static ConstructorHelpers::FObjectFinder<UBlendSpace> Blend(TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/BlendSpaces/Murdock_Locomotion_BS"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> JumpSeq(TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Jump_Apex"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> ReloadSeq(TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Fire_Reload"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> DeathSeq(TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Death_Fwd"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> ReadySeq(TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Fire_NoReload"));
    GetMesh()->SetSkeletalMesh(MeshAsset.Object);
    Locomotion = Blend.Object;
    JumpAnimation = JumpSeq.Object;
    ReloadAnimation = ReloadSeq.Object;
    DeathAnimation = DeathSeq.Object;
    RifleReadyAnimation = ReadySeq.Object;
    FireAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Fire_Fast.Fire_Fast"));
    MovingFireAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Moving_Fire_Fast.Moving_Fire_Fast"));
    MovingReloadAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Moving_Fire_Reload.Moving_Fire_Reload"));
    JumpStartAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Jump_Start.Jump_Start"));
    LandAnimation=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/ParagonMurdock/Characters/Heroes/Murdock/Animations/Jump_Land.Jump_Land"));
    for (const TCHAR* Path : {TEXT("/Game/SoulCityGame/Audio/S_RifleShot01.S_RifleShot01"),TEXT("/Game/SoulCityGame/Audio/S_RifleShot02.S_RifleShot02"),TEXT("/Game/SoulCityGame/Audio/S_RifleShot03.S_RifleShot03")})
        if (auto* Sound=LoadObject<USoundBase>(nullptr,Path)) FireSounds.Add(Sound);
    ReloadSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SoulCityGame/Audio/S_RifleReload.S_RifleReload"));
    FootstepSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/SoulCity/Sound/Wav/Footstep_Metal03.Footstep_Metal03"));
    MuzzleFX=LoadObject<UParticleSystem>(nullptr,TEXT("/Game/ParagonMurdock/FX/Particles/Abilities/Primary/FX/P_Murdock_STD_MUZZLE.P_Murdock_STD_MUZZLE"));
    ImpactFX=LoadObject<UParticleSystem>(nullptr,TEXT("/Game/ParagonMurdock/FX/Particles/Abilities/Primary/FX/P_PlasmaShot_Hit_World.P_PlasmaShot_Hit_World"));
    WeaponAttenuation=CreateDefaultSubobject<USoundAttenuation>(TEXT("WeaponAudioFalloff"));
    WeaponAttenuation->Attenuation.bAttenuate=true;
    WeaponAttenuation->Attenuation.bSpatialize=true;
    WeaponAttenuation->Attenuation.AttenuationShapeExtents=FVector(250,0,0);
    WeaponAttenuation->Attenuation.FalloffDistance=5500.f;
    AIControllerClass = AAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::Disabled;
}

void ACityCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnTime = GetWorld()->GetTimeSeconds();
    LastSafeLocation=GetActorLocation();
    if (bIsBot) FillLight->SetLightColor(FLinearColor(1.f,0.12f,0.08f));
    if (auto* MeshAsset = GetMesh()->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& Skeleton = MeshAsset->GetRefSkeleton();
        for (int32 I = 0; I < Skeleton.GetNum(); ++I)
        {
            const FName Name = Skeleton.GetBoneName(I);
            const FString BoneName=Name.ToString();
            // Hide only the weapon root. Individual gun_* bones include animation
            // controls; hiding those separately leaves the cannon in pieces.
            if (BoneName == TEXT("gun")) WeaponBones.Add(Name);
            if (BoneName == TEXT("shotgun_base")) GetMesh()->HideBoneByName(Name,EPhysBodyOp::PBO_None);
        }
    }
    OnRep_Sprint();
    OnRep_Combat();
    if (Locomotion) UE_LOG(LogTemp, Display, TEXT("CITY_ANIM axes: %s (%g..%g), %s (%g..%g)"),
        *Locomotion->GetBlendParameter(0).DisplayName, Locomotion->GetBlendParameter(0).Min, Locomotion->GetBlendParameter(0).Max,
        *Locomotion->GetBlendParameter(1).DisplayName, Locomotion->GetBlendParameter(1).Min, Locomotion->GetBlendParameter(1).Max);
    if (HasAuthority() && bIsBot)
    {
        FireInterval=0.5f;
        GiveRifle(300);
        SpawnDefaultController();
        GetWorldTimerManager().SetTimer(BotTimer, this, &ACityCharacter::BotThink, 0.18f, true, 1.0f);
    }
}

void ACityCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACityCharacter, Health); DOREPLIFETIME(ACityCharacter, Ammo);
    DOREPLIFETIME(ACityCharacter, ReserveAmmo); DOREPLIFETIME(ACityCharacter, bEquipped);
    DOREPLIFETIME(ACityCharacter, bReloading); DOREPLIFETIME(ACityCharacter, bIsBot);
    DOREPLIFETIME(ACityCharacter, bDead); DOREPLIFETIME(ACityCharacter, bSprinting);
    DOREPLIFETIME(ACityCharacter,bVaulting);
    DOREPLIFETIME_CONDITION(ACityCharacter,bAiming,COND_SkipOwner);
}

void ACityCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis("MoveForward", this, &ACityCharacter::MoveForward);
    Input->BindAxis("MoveRight", this, &ACityCharacter::MoveRight);
    Input->BindAxis("Turn", this, &ACityCharacter::Turn);
    Input->BindAxis("LookUp", this, &ACityCharacter::LookUp);
    Input->BindAction("Jump", IE_Pressed, this, &ACityCharacter::JumpOrVault);
    Input->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    Input->BindAction("Fire", IE_Pressed, this, &ACityCharacter::StartFire);
    Input->BindAction("Fire", IE_Released, this, &ACityCharacter::StopFire);
    Input->BindAction("Reload", IE_Pressed, this, &ACityCharacter::Reload);
    Input->BindAction("Interact", IE_Pressed, this, &ACityCharacter::Interact);
    Input->BindAction("Sprint", IE_Pressed, this, &ACityCharacter::SprintPressed);
    Input->BindAction("Sprint", IE_Released, this, &ACityCharacter::SprintReleased);
    Input->BindAction("Aim", IE_Pressed, this, &ACityCharacter::AimPressed);
    Input->BindAction("Aim", IE_Released, this, &ACityCharacter::AimReleased);
    Input->BindKey(EKeys::V,IE_Pressed,this,&ACityCharacter::ToggleAim);
    Input->BindKey(EKeys::H,IE_Pressed,this,&ACityCharacter::ServerRecover);
}
void ACityCharacter::MoveForward(float V) { if (Controller && IsAlive() && !bVaulting) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::X), V); }
void ACityCharacter::MoveRight(float V) { if (Controller && IsAlive() && !bVaulting) AddMovementInput(FRotationMatrix(FRotator(0, Controller->GetControlRotation().Yaw, 0)).GetUnitAxis(EAxis::Y), V); }
void ACityCharacter::Turn(float V) { if (IsAlive()) AddControllerYawInput(V*(IsScoped()?0.3f:1.f)); }
void ACityCharacter::LookUp(float V) { if (IsAlive()) AddControllerPitchInput(V*(IsScoped()?0.3f:1.f)); }
void ACityCharacter::SprintPressed() { bSprinting = true; OnRep_Sprint(); ServerSprint(true); }
void ACityCharacter::SprintReleased() { bSprinting = false; OnRep_Sprint(); ServerSprint(false); }
void ACityCharacter::ServerSprint_Implementation(bool Enabled) { bSprinting = Enabled && IsAlive(); OnRep_Sprint(); }
void ACityCharacter::OnRep_Sprint() { GetCharacterMovement()->MaxWalkSpeed = bSprinting ? SprintSpeed : WalkSpeed; }
void ACityCharacter::AimPressed() { bAiming = true; ServerAim(true); }
void ACityCharacter::AimReleased() { bAiming = false; ServerAim(false); }
void ACityCharacter::ToggleAim() { if (bAiming) AimReleased(); else AimPressed(); }
void ACityCharacter::ServerAim_Implementation(bool Enabled) { bAiming=Enabled && IsAlive(); }

bool ACityCharacter::FindVault(FVector& Landing,float& Top) const
{
    if (!IsAlive() || bVaulting || GetCharacterMovement()->IsFalling()) return false;
    const FVector Forward=GetActorForwardVector().GetSafeNormal2D();
    const float Half=GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const float Feet=GetActorLocation().Z-Half;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityVault),false,this);
    FHitResult Front,Upper,Floor,Clear;
    const FVector From(GetActorLocation().X,GetActorLocation().Y,Feet+45);
    if (!GetWorld()->LineTraceSingleByChannel(Front,From,From+Forward*110.f,ECC_WorldStatic,Params)) return false;
    const FVector Above=Front.ImpactPoint+Forward*18.f;
    if (!GetWorld()->LineTraceSingleByChannel(Upper,FVector(Above.X,Above.Y,Feet+190),FVector(Above.X,Above.Y,Feet+55),ECC_WorldStatic,Params)) return false;
    const float Height=Upper.ImpactPoint.Z-Feet;
    if (Height<60.f || Height>180.f || Upper.ImpactNormal.Z<0.65f) return false;
    const FVector Beyond=Front.ImpactPoint+Forward*155.f;
    if (!GetWorld()->LineTraceSingleByChannel(Floor,FVector(Beyond.X,Beyond.Y,Upper.ImpactPoint.Z+60),FVector(Beyond.X,Beyond.Y,Feet-160),ECC_WorldStatic,Params)) return false;
    if (Floor.ImpactNormal.Z<0.65f) return false;
    const bool Mantle=Floor.ImpactPoint.Z>Feet+55.f;
    if (Mantle && FMath::Abs(Floor.ImpactPoint.Z-Upper.ImpactPoint.Z)>25.f) return false;
    if (!Mantle && Height>145.f) return false;
    Landing=Floor.ImpactPoint+FVector(0,0,Half+3.f);
    const FCollisionShape Capsule=FCollisionShape::MakeCapsule(34.f,Half);
    if (GetWorld()->OverlapBlockingTestByChannel(Landing,FQuat::Identity,ECC_Pawn,Capsule,Params)) return false;
    Top=Upper.ImpactPoint.Z+Half+12.f;
    const FVector HighStart(GetActorLocation().X,GetActorLocation().Y,Top), HighEnd(Landing.X,Landing.Y,Top);
    if (GetWorld()->SweepSingleByChannel(Clear,GetActorLocation(),HighStart,FQuat::Identity,ECC_Pawn,Capsule,Params)) return false;
    if (GetWorld()->SweepSingleByChannel(Clear,HighStart,HighEnd,FQuat::Identity,ECC_Pawn,Capsule,Params)) return false;
    if (GetWorld()->SweepSingleByChannel(Clear,HighEnd,Landing,FQuat::Identity,ECC_Pawn,Capsule,Params)) return false;
    return true;
}
void ACityCharacter::JumpOrVault()
{
    if (!IsAlive() || bVaulting) return;
    FVector Landing; float Top;
    if (FindVault(Landing,Top)) ServerTryVault(); else Jump();
}
void ACityCharacter::ServerTryVault_Implementation()
{
    FVector Landing; float Top;
    if (!FindVault(Landing,Top)) return;
    ServerStopFire_Implementation();
    MulticastVault(GetActorLocation(),Landing,Top,GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
}
void ACityCharacter::MulticastVault_Implementation(FVector Start,FVector Target,float Top,float ServerTime)
{
    VaultStart=Start; VaultTarget=Target; VaultTop=Top; VaultStartTime=ServerTime; bVaulting=true;
    GetCharacterMovement()->StopMovementImmediately(); GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}
void ACityCharacter::MulticastEndVault_Implementation(FVector Location)
{
    bVaulting=false; SetActorLocation(Location);
    if (IsAlive()) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
void ACityCharacter::ServerRecover_Implementation()
{
    const float Now=GetWorld()->GetTimeSeconds();
    if (!IsAlive() || bIsBot || Now-LastRecoveryTime<5.f) return;
    FVector Location=LastSafeLocation;
    if (!GetWorld()->FindTeleportSpot(this,Location,GetActorRotation())) return;
    ServerStopFire_Implementation();
    GetCharacterMovement()->StopMovementImmediately();
    MulticastEndVault(Location);
    LastRecoveryTime=Now; DisconnectedSince=0;
    if (auto* PC=Cast<APlayerController>(Controller))
    {
        const FRotator Facing(0,GetActorRotation().Yaw,0);
        PC->SetControlRotation(Facing); PC->ClientSetRotation(Facing);
    }
    ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("CITY_RECOVERY location=%s"),*Location.ToString());
}

void ACityCharacter::Tick(float DT)
{
    Super::Tick(DT);
    if (bVaulting && IsAlive())
    {
        const float Now=GetWorld()->GetGameState() ? GetWorld()->GetGameState()->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
        const float T=FMath::Clamp((Now-VaultStartTime)/0.7f,0.f,1.f);
        FVector Position;
        const FVector HighStart(VaultStart.X,VaultStart.Y,VaultTop), HighEnd(VaultTarget.X,VaultTarget.Y,VaultTop);
        if (T<0.3f) Position=FMath::Lerp(VaultStart,HighStart,FMath::SmoothStep(0.f,1.f,T/0.3f));
        else if (T<0.75f) Position=FMath::Lerp(HighStart,HighEnd,FMath::SmoothStep(0.f,1.f,(T-0.3f)/0.45f));
        else Position=FMath::Lerp(HighEnd,VaultTarget,FMath::SmoothStep(0.f,1.f,(T-0.75f)/0.25f));
        FHitResult Hit; SetActorLocation(Position,HasAuthority(),&Hit);
        if (HasAuthority() && (T>=1.f || Hit.bBlockingHit)) MulticastEndVault(GetActorLocation());
    }
#if !UE_BUILD_SHIPPING
    if (IsLocallyControlled() && !bIsBot && FParse::Param(FCommandLine::Get(),TEXT("CityArenaTest")) && GetWorld()->GetTimeSeconds()>=2.5f && GetWorld()->GetTimeSeconds()<4.6f)
        AddMovementInput(FVector::ForwardVector,1.f);
    if (IsLocallyControlled() && !bIsBot && FParse::Param(FCommandLine::Get(),TEXT("CityConnectivityTest")) && GetWorld()->GetTimeSeconds()>=2.f && GetWorld()->GetTimeSeconds()<9.f)
        AddMovementInput(FVector::ForwardVector,1.f);
    if (IsLocallyControlled() && !bIsBot && FParse::Param(FCommandLine::Get(),TEXT("CityPitTest")) && GetWorld()->GetTimeSeconds()>=2.f && GetWorld()->GetTimeSeconds()<3.6f)
        AddMovementInput(FVector::RightVector,1.f);
    if (IsLocallyControlled() && !bIsBot && FParse::Param(FCommandLine::Get(),TEXT("CityPitTest")) && GetWorld()->GetTimeSeconds()>=4.2f && GetWorld()->GetTimeSeconds()<6.5f)
        AddMovementInput(FVector(-.894427f,-.447214f,0),1.f);
#endif
    if (GetNetMode() != NM_DedicatedServer) UpdateVisuals(DT);
    if (!bIsBot)
    {
        Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,IsScoped()?28.f:90.f,DT,16.f));
        CameraBoom->TargetArmLength=FMath::FInterpTo(CameraBoom->TargetArmLength,IsScoped()?0.f:320.f,DT,16.f);
        CameraBoom->SocketOffset=FMath::VInterpTo(CameraBoom->SocketOffset,IsScoped()?FVector(0,0,60):FVector(0,70,70),DT,16.f);
        GetMesh()->SetOwnerNoSee(IsScoped());
    }
    OpticRoot->SetVisibility(bEquipped && IsAlive() && !(IsLocallyControlled() && IsScoped()),true);
    if (HasAuthority() && !bIsBot && IsAlive() && !bVaulting && GetWorld()->GetTimeSeconds()>=NextSafetyCheck)
    {
        const float Now=GetWorld()->GetTimeSeconds(); NextSafetyCheck=Now+0.75f;
        if (GetCharacterMovement()->IsMovingOnGround())
        {
            const bool BelowSafe=LastSafeLocation.Z-GetActorLocation().Z>=120.f;
            bool Connected=!BelowSafe;
            if (!Connected)
            {
                if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
                {
                    FNavLocation A,B;
                    if (auto* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
                    if (Nav->ProjectPointToNavigation(LastSafeLocation,A,FVector(90,90,160)) && Nav->ProjectPointToNavigation(GetActorLocation(),B,FVector(90,90,90)))
                    {
                        const auto Path=Nav->FindPathSync(FPathFindingQuery(this,*Data,A.Location,B.Location));
                        Connected=Path.IsSuccessful() && Path.Path.IsValid() && !Path.Path->IsPartial();
                    }
                }
            }
            if (Connected) { if (!BelowSafe) LastSafeLocation=GetActorLocation(); DisconnectedSince=0; }
            else if (DisconnectedSince<=0) DisconnectedSince=Now;
            else if (Now-DisconnectedSince>3.f) ServerRecover();
        }
    }
    if (HasAuthority() && bIsBot && IsAlive() && BotTarget.IsValid() && BotTarget->IsAlive())
    {
        const FVector Delta = BotTarget->GetActorLocation() - GetActorLocation();
        if (Controller)
        {
            FVector Eye; FRotator View; GetActorEyesViewPoint(Eye,View);
            Controller->SetControlRotation((BotTarget->GetActorLocation()+FVector(0,0,30)-Eye).Rotation());
        }
        if (!bUsingNavigation && Delta.Size2D() > 400.f)
        {
            // Direct steering fallback also works before a navigation mesh is built.
            FVector Direction = Delta.GetSafeNormal2D();
            FHitResult Wall;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(BotSteer), false, this);
            if (GetWorld()->LineTraceSingleByChannel(Wall, GetActorLocation(), GetActorLocation() + Direction * 170, ECC_WorldStatic, Params))
                Direction = FVector::VectorPlaneProject(Direction, Wall.ImpactNormal).GetSafeNormal2D();
            AddMovementInput(Direction, 0.8f);
        }
    }
}

void ACityCharacter::SetVisualAnimation(UAnimationAsset* Asset, bool Loop, float Rate)
{
    if (!Asset) return;
    if (VisualAsset != Asset)
    {
        GetMesh()->PlayAnimation(Asset, Loop);
        if (auto* Anim = GetMesh()->GetSingleNodeInstance()) Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
        VisualAsset = Asset;
    }
    if (auto* Anim = GetMesh()->GetSingleNodeInstance()) Anim->SetPlayRate(Rate);
}
void ACityCharacter::UpdateVisuals(float DT)
{
    if (bDead) { SetVisualAnimation(DeathAnimation, false); return; }
    const float Now=GetWorld()->GetTimeSeconds(),Speed=GetVelocity().Size2D();
    const bool Falling=GetCharacterMovement()->IsFalling();
    if (Falling && !bVisualWasFalling) JumpStartUntil=Now+0.2f;
    if (!Falling && bVisualWasFalling) LandUntil=Now+0.18f;
    bVisualWasFalling=Falling;
    if (bVaulting) { SetVisualAnimation(JumpStartAnimation?JumpStartAnimation:JumpAnimation,true); return; }
    if (Falling) { SetVisualAnimation(Now<JumpStartUntil && JumpStartAnimation?JumpStartAnimation:JumpAnimation,Now>=JumpStartUntil); return; }
    if (Now<LandUntil && LandAnimation) { SetVisualAnimation(LandAnimation,false,LandAnimation->GetPlayLength()/0.18f); return; }
    if (FootstepSound && Speed>80.f && Now>NextFootstep)
    {
        NextFootstep=Now+(bSprinting?0.28f:0.43f);
        UGameplayStatics::PlaySoundAtLocation(this,FootstepSound,GetActorLocation()-FVector(0,0,80),0.2f,FMath::FRandRange(.9f,1.1f),0.f,WeaponAttenuation);
    }
    if (bReloading)
    {
        UAnimSequence* Animation=Speed>10.f && MovingReloadAnimation?MovingReloadAnimation:ReloadAnimation;
        SetVisualAnimation(Animation,false,Animation?Animation->GetPlayLength()/ReloadSeconds:1.f);
        return;
    }
    if (Now<VisualFireUntil)
    {
        UAnimSequence* Animation=Speed>10.f && MovingFireAnimation?MovingFireAnimation:FireAnimation;
        SetVisualAnimation(Animation,false,Animation?Animation->GetPlayLength()/0.23f:1.f);
        return;
    }
    if (bEquipped && GetVelocity().Size2D()<10.f && RifleReadyAnimation)
    {
        SetVisualAnimation(RifleReadyAnimation,false,0.f);
        if (auto* Anim=GetMesh()->GetSingleNodeInstance()) Anim->SetPosition(FMath::Min(0.2f,RifleReadyAnimation->GetPlayLength()*0.5f),false);
        return;
    }
    SetVisualAnimation(Locomotion, true);
    if (Locomotion)
    {
        const FVector Local = GetActorRotation().UnrotateVector(GetVelocity());
        const float Direction = Speed > 5 ? FMath::RadiansToDegrees(FMath::Atan2(Local.Y, Local.X)) : 0.f;
        auto Axis = [&](int32 I) {
            const FBlendParameter& P = Locomotion->GetBlendParameter(I);
            return P.DisplayName.Contains(TEXT("direction"), ESearchCase::IgnoreCase) || P.Min < -90.f ? Direction : Speed;
        };
        if (auto* Anim = GetMesh()->GetSingleNodeInstance())
        {
            Anim->SetBlendSpacePosition(FVector(Axis(0), Axis(1), 0));
            // The supplied jog samples are authored for 360 cm/s. Sprint must speed up the gait too.
            Anim->SetPlayRate(FMath::Max(1.f, Speed / 360.f));
        }
    }
}
void ACityCharacter::OnRep_Combat()
{
    if (bVisualEquipped != bEquipped || WeaponBones.Num())
    {
        for (FName Bone : WeaponBones)
            if (bEquipped) GetMesh()->UnHideBoneByName(Bone); else GetMesh()->HideBoneByName(Bone, EPhysBodyOp::PBO_None);
        bVisualEquipped = bEquipped;
    }
    if (bDead && !bVisualDead)
    {
        bVisualDead = true;
        GetCharacterMovement()->DisableMovement();
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (FillLight) FillLight->SetVisibility(false);
    }
}
void ACityCharacter::StartFire() { if (IsAlive()) ServerStartFire(); }
void ACityCharacter::StopFire() { ServerStopFire(); }
void ACityCharacter::ServerStartFire_Implementation()
{
    if (!IsAlive() || !bEquipped || bReloading || bVaulting || GetWorldTimerManager().IsTimerActive(FireTimer)) return;
    FireShot();
    GetWorldTimerManager().SetTimer(FireTimer, this, &ACityCharacter::FireShot, FireInterval, true);
}
void ACityCharacter::ServerStopFire_Implementation() { GetWorldTimerManager().ClearTimer(FireTimer); }
void ACityCharacter::FireShot()
{
    if (!HasAuthority() || !IsAlive() || !bEquipped || bReloading) { GetWorldTimerManager().ClearTimer(FireTimer); return; }
    if (Ammo <= 0) { GetWorldTimerManager().ClearTimer(FireTimer); ServerReload_Implementation(); return; }
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastShotTime < FireInterval * 0.9f) return;
    LastShotTime = Now;
    --Ammo;
    FVector Eye; FRotator Aim;
    GetActorEyesViewPoint(Eye, Aim);
    FVector AimDirection = Aim.Vector();
    if (bIsBot) AimDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(3.5f));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(CityRifle), true, this);
    FVector Target = Eye + AimDirection * 12000.f;
    FHitResult AimHit;
    if (bIsBot || Camera)
    {
        const FVector CameraStart = bIsBot?Eye:Camera->GetComponentLocation();
        if (GetWorld()->LineTraceSingleByChannel(AimHit, CameraStart, CameraStart + AimDirection * 12000.f, ECC_Visibility, Params)) Target = AimHit.ImpactPoint;
    }
    const FVector Muzzle = IsScoped() ? Eye+AimDirection*40.f+GetActorRightVector()*12.f : GetMesh()->GetSocketLocation(TEXT("gun_barrel"));
    FHitResult Hit;
    // Continue beyond the camera's surface point: ending exactly on that point
    // can miss a capsule due to precision and the muzzle's sideways offset.
    const FVector TraceEnd=Target+(Target-Muzzle).GetSafeNormal()*50.f;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Muzzle, TraceEnd, ECC_Visibility, Params);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("CityParkourTest"))) UE_LOG(LogTemp,Display,TEXT("CITY_SCOPE_TRACE eye=%s camera=%s muzzle=%s target=%s hit=%s"),*Eye.ToString(),*Camera->GetComponentLocation().ToString(),*Muzzle.ToString(),*Target.ToString(),*GetNameSafe(Hit.GetActor()));
#endif
    const FVector End = bHit ? Hit.ImpactPoint : Target;
    if (auto* Victim = Cast<ACityCharacter>(Hit.GetActor()))
    {
        if (Victim->bIsBot != bIsBot) UGameplayStatics::ApplyPointDamage(Victim, bIsBot ? 8.f : 25.f, AimDirection, Hit, GetController(), this, UDamageType::StaticClass());
    }
    MulticastShot(Muzzle, End, bHit);
    ForceNetUpdate();
}
void ACityCharacter::MulticastShot_Implementation(FVector_NetQuantize Start, FVector_NetQuantize End, bool Hit)
{
    if (GetNetMode() == NM_DedicatedServer) return;
    VisualFireUntil=GetWorld()->GetTimeSeconds()+0.23f;
    UAnimSequence* ShotAnimation=GetVelocity().Size2D()>10.f && MovingFireAnimation?MovingFireAnimation:FireAnimation;
    if (ShotAnimation)
    {
        GetMesh()->PlayAnimation(ShotAnimation,false); VisualAsset=ShotAnimation;
        if (auto* Anim=GetMesh()->GetSingleNodeInstance()) { Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion); Anim->SetPlayRate(ShotAnimation->GetPlayLength()/0.23f); }
    }
    if (!FApp::CanEverRender()) return;
    if (FireSounds.Num()) UGameplayStatics::PlaySoundAtLocation(this,FireSounds[FMath::RandRange(0,FireSounds.Num()-1)],Start,bIsBot?0.65f:0.8f,FMath::FRandRange(.95f,1.05f),0.f,WeaponAttenuation);
    if (MuzzleFX && !(IsLocallyControlled() && IsScoped())) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),MuzzleFX,Start,(FVector(End)-FVector(Start)).Rotation(),FVector(.5f),true);
    if (auto* Tracer=GetWorld()->SpawnActor<ACityBulletVisual>(Start,FRotator::ZeroRotator)) Tracer->Configure(Start,End,Hit?ImpactFX.Get():nullptr,bIsBot);
    if (IsLocallyControlled() && !bIsBot) AddControllerPitchInput(-0.13f);
}
void ACityCharacter::MulticastReloadAudio_Implementation()
{
    if (GetNetMode()!=NM_DedicatedServer && ReloadSound) UGameplayStatics::PlaySoundAtLocation(this,ReloadSound,GetActorLocation(),0.7f,1.f,0.f,WeaponAttenuation);
}
void ACityCharacter::Reload() { ServerReload(); }
void ACityCharacter::ServerReload_Implementation()
{
    if (!IsAlive() || !bEquipped || bReloading || Ammo >= MagazineSize || ReserveAmmo <= 0) return;
    GetWorldTimerManager().ClearTimer(FireTimer);
    bReloading = true;
    MulticastReloadAudio();
    ForceNetUpdate();
    GetWorldTimerManager().SetTimer(ReloadTimer, this, &ACityCharacter::FinishReload, ReloadSeconds, false);
}
void ACityCharacter::FinishReload()
{
    if (!HasAuthority() || !IsAlive()) return;
    const int32 Added = FMath::Min(MagazineSize - Ammo, ReserveAmmo);
    Ammo += Added; ReserveAmmo -= Added; bReloading = false;
    ForceNetUpdate();
}
ACityPickup* ACityCharacter::FindPickup() const
{
    ACityPickup* Nearest = nullptr;
    float Best = 225.f * 225.f;
    for (TActorIterator<ACityPickup> It(GetWorld()); It; ++It)
    {
        if (It->bCollected) continue;
        const float Distance = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (Distance < Best)
        {
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(PickupLOS), false, this);
            if (!GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation(), It->GetActorLocation() + FVector(0,0,30), ECC_WorldStatic, Params))
            { Best = Distance; Nearest = *It; }
        }
    }
    return Nearest;
}
void ACityCharacter::Interact() { ServerInteract(); }
void ACityCharacter::ServerInteract_Implementation() { if (IsAlive()) if (auto* Pickup = FindPickup()) Pickup->TryCollect(this); }
void ACityCharacter::GiveRifle(int32 Spare)
{
    if (!HasAuthority()) return;
    if (!bEquipped) { bEquipped = true; Ammo = MagazineSize; ReserveAmmo = Spare; }
    else ReserveAmmo = FMath::Min(ReserveAmmo + Spare, 300);
    OnRep_Combat(); ForceNetUpdate();
}
float ACityCharacter::TakeDamage(float Amount, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer)
{
    if (!HasAuthority() || !IsAlive() || Amount <= 0) return 0;
    if (!bIsBot && GetWorld()->GetTimeSeconds() - SpawnTime < 5.f) return 0;
    if (auto* Attacker = Cast<ACityCharacter>(Causer)) if (Attacker->bIsBot == bIsBot) return 0;
    const float Applied = FMath::Min(Amount, Health);
    Health -= Applied;
    if (Health <= 0) Die(InstigatorController);
    OnRep_Combat(); ForceNetUpdate();
    return Applied;
}
void ACityCharacter::Die(AController* Killer)
{
    bDead = true; bReloading = false;
    GetWorldTimerManager().ClearTimer(FireTimer);
    GetWorldTimerManager().ClearTimer(ReloadTimer);
    GetWorldTimerManager().ClearTimer(BotTimer);
    if (auto* AI = Cast<AAIController>(Controller)) AI->StopMovement();
    if (auto* GM = GetWorld()->GetAuthGameMode<ACityGameMode>()) GM->CharacterDied(this,Killer);
    if (bIsBot) SetLifeSpan(8.f);
}
void ACityCharacter::FellOutOfWorld(const UDamageType& DamageType)
{
    if (HasAuthority() && IsAlive()) { Health=0; Die(nullptr); OnRep_Combat(); ForceNetUpdate(); }
}
void ACityCharacter::BotThink()
{
    if (!HasAuthority() || !IsAlive()) return;
    float Best = FLT_MAX;
    ACityCharacter* Target = nullptr;
    for (TActorIterator<ACityCharacter> It(GetWorld()); It; ++It)
    {
        if (It->bIsBot || !It->IsAlive()) continue;
        float Distance = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
        if (Distance < Best) { Best = Distance; Target = *It; }
    }
    BotTarget = Target;
    if (!Target) { ServerStopFire_Implementation(); return; }
    bUsingNavigation=false;
    if (auto* AI=Cast<AAIController>(Controller))
    {
        if (auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation Projected;
            if (Nav->ProjectPointToNavigation(GetActorLocation(),Projected,FVector(120,120,250)))
            {
                bUsingNavigation=true;
                if (Best>FMath::Square(650.f)) bUsingNavigation=AI->MoveToActor(Target,500.f)!=EPathFollowingRequestResult::Failed;
                else if (GetWorld()->GetTimeSeconds()>BotRetargetTime)
                {
                    FNavLocation Flank;
                    if (Nav->GetRandomReachablePointInRadius(GetActorLocation(),450.f,Flank)) AI->MoveToLocation(Flank.Location,60.f);
                    BotRetargetTime=GetWorld()->GetTimeSeconds()+2.5f;
                }
            }
        }
    }
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BotSight), false, this);
    GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0,0,60), Target->GetActorLocation() + FVector(0,0,40), ECC_Visibility, Params);
    if (Hit.GetActor() == Target && Best < 3000.f * 3000.f) ServerStartFire_Implementation(); else ServerStopFire_Implementation();
    if (Ammo == 0) ServerReload_Implementation();
}

ACityPickup::ACityPickup()
{
    bReplicates = true;
    Case = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleBody"));
    SetRootComponent(Case);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube"));
    Case->SetStaticMesh(Cube.Object);
    Case->SetWorldScale3D(FVector(0.65f, 0.14f, 0.22f));
    Case->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Barrel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RifleBarrel"));
    Barrel->SetupAttachment(Case);
    Barrel->SetStaticMesh(Cube.Object);
    Barrel->SetRelativeLocation(FVector(70,0,25));
    Barrel->SetRelativeScale3D(FVector(0.7f,0.4f,0.4f));
    Barrel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (auto* Rifle=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/SoulCityGame/Weapons/SM_MurdockPrimaryRifle.SM_MurdockPrimaryRifle")))
    {
        Case->SetStaticMesh(Rifle); Case->SetWorldScale3D(FVector(1.65f));
        Barrel->SetVisibility(false);
    }
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PickupLabel"));
    Label->SetupAttachment(Case);
    Label->SetAbsolute(false, false, true);
    Label->SetWorldScale3D(FVector::OneVector);
    Label->SetRelativeLocation(FVector(0,0,180));
    Label->SetText(FText::FromString(TEXT("RIFLE / AMMO  [E]")));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(20.f);
    Label->SetTextRenderColor(FColor::Cyan);
    Label->SetVisibility(false); // The screen-space interaction prompt stays readable from either side.
    Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("PickupGlow"));
    Light->SetupAttachment(Case);
    Light->SetRelativeLocation(FVector(0,0,70));
    Light->SetLightColor(FLinearColor(0.1f,0.7f,1.f));
    Light->Intensity = 4.f; Light->AttenuationRadius = 120.f;
    Light->SetCastShadows(false);
}
bool ACityPickup::TryCollect(ACityCharacter* Player)
{
    if (!HasAuthority() || bCollected || !IsValid(Player) || !Player->IsAlive() || FVector::DistSquared(Player->GetActorLocation(), GetActorLocation()) > FMath::Square(225.f)) return false;
    if (Player->bEquipped && Player->ReserveAmmo >= 300) return false;
    bCollected = true; Player->GiveRifle(); Destroy(); return true;
}

ACityBulletVisual::ACityBulletVisual()
{
    PrimaryActorTick.bCanEverTick=true;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlasmaTracer")); SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder"));
    Mesh->SetStaticMesh(Cylinder.Object); Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCastShadow(false); Mesh->SetRelativeScale3D(FVector(.018f,.018f,.35f));
    InitialLifeSpan=1.f;
}
void ACityBulletVisual::Configure(FVector Start,FVector End,UParticleSystem* Impact,bool Enemy)
{
    Origin=Start; Direction=(End-Start).GetSafeNormal(); Length=FVector::Distance(Start,End); ImpactTemplate=Impact;
    SetActorRotation(FRotationMatrix::MakeFromZ(Direction).Rotator());
    if (auto* Material=LoadObject<UMaterialInterface>(nullptr,Enemy?TEXT("/Game/SoulCityGame/Weapons/Materials/M_TracerRed.M_TracerRed"):TEXT("/Game/SoulCityGame/Weapons/Materials/M_TracerBlue.M_TracerBlue"))) Mesh->SetMaterial(0,Material);
}
void ACityBulletVisual::Tick(float DT)
{
    Super::Tick(DT); Travel+=DT*18000.f;
    SetActorLocation(Origin+Direction*FMath::Min(Travel,Length));
    if (Travel>=Length)
    {
        if (ImpactTemplate) UGameplayStatics::SpawnEmitterAtLocation(GetWorld(),ImpactTemplate,GetActorLocation(),(-Direction).Rotation(),FVector(.4f),true);
        Destroy();
    }
}
void ACityGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACityGameState, Wave); DOREPLIFETIME(ACityGameState, BotsAlive); DOREPLIFETIME(ACityGameState, TeamKills);
    DOREPLIFETIME(ACityGameState, RoundsWon); DOREPLIFETIME(ACityGameState, NextWaveTime);
    DOREPLIFETIME(ACityGameState,bCombatMode);
}
void ACityPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ACityPlayerState,Kills); DOREPLIFETIME(ACityPlayerState,Deaths);
}

ACityGameMode::ACityGameMode()
{
    DefaultPawnClass = ACityCharacter::StaticClass();
    PlayerControllerClass = ACityPlayerController::StaticClass();
    HUDClass = ACityHUD::StaticClass();
    GameStateClass = ACityGameState::StaticClass();
    PlayerStateClass = ACityPlayerState::StaticClass();
}
bool ACityGameMode::FindFloor(const FVector& Wanted, FVector& Out) const
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(SupplyFloor), true);
    if (GetWorld()->LineTraceSingleByObjectType(Hit, Wanted + FVector(0,0,130), Wanted - FVector(0,0,500), FCollisionObjectQueryParams(ECC_WorldStatic), Params) && Hit.ImpactNormal.Z > 0.65f)
    { Out = Hit.ImpactPoint; return true; }
    return false;
}
void ACityGameMode::BeginPlay()
{
    Super::BeginPlay();
    int32 StartCount=0;
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) { ArenaOrigin += It->GetActorLocation(); ++StartCount; }
    if (StartCount) ArenaOrigin /= StartCount;
    SpawnSupplies(ArenaOrigin);
    const bool Combat=!UGameplayStatics::GetCurrentLevelName(this,true).Contains(TEXT("CityOpen"));
    if (auto* State=GetGameState<ACityGameState>())
    {
        State->bCombatMode=Combat;
        State->NextWaveTime=Combat?State->GetServerWorldTimeSeconds()+6.f:0.f;
    }
    if (!Combat) return;
    if (!FParse::Param(FCommandLine::Get(), TEXT("CityNetTest")) && !FParse::Param(FCommandLine::Get(), TEXT("CityVisualTest")) && !FParse::Param(FCommandLine::Get(),TEXT("CityParkourTest")) && !FParse::Param(FCommandLine::Get(),TEXT("CityMapTravelTest")) && !FParse::Param(FCommandLine::Get(),TEXT("CityConnectivityTest")))
        GetWorldTimerManager().SetTimer(WaveTimer, this, &ACityGameMode::StartWave, FParse::Param(FCommandLine::Get(),TEXT("CityArenaTest")) ? 4.f : 6.f, false);
#if !UE_BUILD_SHIPPING
    else
        GetWorldTimerManager().SetTimer(WaveTimer, []() { FPlatformMisc::RequestExit(false); }, FParse::Param(FCommandLine::Get(),TEXT("CityVisualTest")) ? 90.f : 45.f, false);
#endif
}
void ACityGameMode::SpawnSupplies(const FVector& Around)
{
    int32 Existing=0;
    for (TActorIterator<ACityPickup> It(GetWorld()); It; ++It) if (!It->bCollected) ++Existing;
    if (Existing >= 4) return;
    TArray<FVector> Starts;
    for (TActorIterator<APlayerStart> It(GetWorld());It;++It) Starts.Add(It->GetActorLocation());
    if (!Starts.Num()) Starts.Add(Around);
    TArray<TPair<FVector,FVector>> Candidates;
    for (FVector Offset : { FVector(110,0,0),FVector(180,-60,0),FVector(-100,0,0),FVector(160,40,0) })
        for (FVector Start:Starts) Candidates.Emplace(Start,Start+Offset);
    for (const auto& Candidate:Candidates)
    {
        if (Existing >= 4) break;
        FVector Floor;
        if (FindFloor(Candidate.Value, Floor))
        {
            const auto* PickupDefaults=GetDefault<ACityPickup>();
            const float Height=PickupDefaults->Case->GetStaticMesh() ? PickupDefaults->Case->GetStaticMesh()->GetBounds().BoxExtent.Z*1.65f+3.f : 22.f;
            FHitResult Block;
            if (GetWorld()->LineTraceSingleByObjectType(Block,Candidate.Key,Floor+FVector(0,0,Height+30),FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionQueryParams(SCENE_QUERY_STAT(SupplyVisibility),false))) continue;
            bool Occupied=false;
            for (TActorIterator<ACityPickup> It(GetWorld()); It; ++It) if (FVector::DistSquared(It->GetActorLocation(),Floor)<FMath::Square(70.f)) { Occupied=true; break; }
            if (!Occupied && GetWorld()->SpawnActor<ACityPickup>(Floor + FVector(0,0,Height), FRotator(0,30,0))) ++Existing;
        }
    }
}
void ACityGameMode::PostLogin(APlayerController* Player)
{
    Super::PostLogin(Player);
    // All humans share one team; their spawn is selected by Unreal's normal collision-aware spawn handling.
    if (Player->PlayerState) Player->PlayerState->SetPlayerName(FString::Printf(TEXT("Oyuncu %d"),GetNumPlayers()));
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("CityNetTest")))
    {
        FTimerHandle Handle;
        TWeakObjectPtr<APlayerController> WeakPlayer(Player);
        GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [this,WeakPlayer]() {
            if (WeakPlayer.IsValid() && WeakPlayer->GetPawn())
            {
                if (FParse::Param(FCommandLine::Get(),TEXT("CityVaultNetTest")) && !WeakPlayer->IsLocalController())
                {
                    const bool Other=WeakPlayer->PlayerState && WeakPlayer->PlayerState->GetPlayerName().Contains(TEXT("3"));
                    WeakPlayer->GetPawn()->SetActorLocation(FVector(-2500,Other?2075:1925,90.15f));
                    WeakPlayer->SetControlRotation(FRotator::ZeroRotator);
                }
                GetWorld()->SpawnActor<ACityPickup>(WeakPlayer->GetPawn()->GetActorLocation() + FVector(0,0,40), FRotator::ZeroRotator);
            }
        }), 1.f, false);
    }
#endif
}
void ACityGameMode::StartWave()
{
    auto* State = GetGameState<ACityGameState>();
    if (!State) return;
    if (!State->bCombatMode) return;
    const int32 NextWave=State->Wave+1;
    State->NextWaveTime=0;
    const int32 Count = FMath::Min(2 + NextWave, 8);
    TArray<FVector> SpawnPoints;
    for (TActorIterator<ATargetPoint> It(GetWorld());It;++It) if (It->ActorHasTag(TEXT("CityEnemySpawn"))) SpawnPoints.Add(It->GetActorLocation());
    int32 Spawned = 0;
    for (int32 Attempt = 0; Attempt < 80 && Spawned < Count; ++Attempt)
    {
        float Angle = Attempt * 2.399963f;
        float Radius = 3000.f + (Attempt % 5) * 500.f;
        const bool Marked=Attempt<SpawnPoints.Num();
        const FVector Wanted=Marked?SpawnPoints[Attempt]:ArenaOrigin+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
        FVector Ground;
        if (!FindFloor(Wanted, Ground)) continue;
        if (Marked && FMath::Abs(Ground.Z-(Wanted.Z-95.f))>65.f) continue;
        bool TooClose=false;
        for (TActorIterator<ACityCharacter> It(GetWorld());It;++It)
        {
            if (!It->IsAlive()) continue;
            if (FVector::Dist2D(It->GetActorLocation(),Ground)<(It->bIsBot?250.f:2600.f)) { TooClose=true; break; }
        }
        if (TooClose) continue;
        FVector Location = Ground + FVector(0,0,95);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(BotSpawn), false);
        if (GetWorld()->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(35,88), Params)) continue;
        FTransform Transform(FRotator(0,(ArenaOrigin-Location).Rotation().Yaw,0),Location);
        ACityCharacter* Bot = GetWorld()->SpawnActorDeferred<ACityCharacter>(ACityCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
        if (Bot)
        {
            Bot->bIsBot = true; Bot->WalkSpeed = 200.f;
            UGameplayStatics::FinishSpawningActor(Bot, Transform);
            float NearestHuman=FLT_MAX;
            for (TActorIterator<ACityCharacter> It(GetWorld());It;++It) if (!It->bIsBot && It->IsAlive()) NearestHuman=FMath::Min(NearestHuman,FVector::Dist2D(It->GetActorLocation(),Bot->GetActorLocation()));
            UE_LOG(LogTemp,Display,TEXT("CITY_BOT_SPAWN distance=%g location=%s"),NearestHuman,*Bot->GetActorLocation().ToString());
#if !UE_BUILD_SHIPPING
            if (FParse::Param(FCommandLine::Get(),TEXT("CityArenaTest")) || FParse::Param(FCommandLine::Get(),TEXT("CityBattleTest")))
            {
                FTimerHandle MovementCheck;
                TWeakObjectPtr<ACityCharacter> WeakBot(Bot);
                GetWorldTimerManager().SetTimer(MovementCheck,FTimerDelegate::CreateWeakLambda(this,[WeakBot,Location]() {
                    const float Travel=WeakBot.IsValid() ? FVector::Dist2D(WeakBot->GetActorLocation(),Location) : 0;
                    UE_LOG(LogTemp,Display,TEXT("CITY_BOT_MOVEMENT %s distance=%g"),Travel>100?TEXT("PASS"):TEXT("FAIL"),Travel);
                }),5.f,false);
            }
#endif
            ++Spawned;
        }
    }
    State->BotsAlive += Spawned;
    if (Spawned) State->Wave=NextWave;
    UE_LOG(LogTemp, Display, TEXT("CITY_WAVE %d spawned=%d"), State->Wave, Spawned);
    if (!Spawned) { State->NextWaveTime=State->GetServerWorldTimeSeconds()+3.f; GetWorldTimerManager().SetTimer(WaveTimer, this, &ACityGameMode::StartWave, 3.f, false); }
}
void ACityGameMode::CharacterDied(ACityCharacter* Victim, AController* Killer)
{
    if (Victim->bIsBot)
    {
        if (auto* State = GetGameState<ACityGameState>())
        {
            State->BotsAlive = FMath::Max(0, State->BotsAlive - 1); ++State->TeamKills;
            if (Killer) if (auto* PS=Killer->GetPlayerState<ACityPlayerState>()) ++PS->Kills;
            if (State->BotsAlive == 0) { ++State->RoundsWon; State->NextWaveTime=State->GetServerWorldTimeSeconds()+8.f; SpawnSupplies(ArenaOrigin); GetWorldTimerManager().SetTimer(WaveTimer, this, &ACityGameMode::StartWave, 8.f, false); }
        }
    }
    else if (AController* PC = Victim->GetController())
    {
        if (auto* PS=PC->GetPlayerState<ACityPlayerState>()) ++PS->Deaths;
        FTimerHandle Respawn;
        TWeakObjectPtr<AController> WeakPC(PC);
        TWeakObjectPtr<ACityCharacter> WeakPawn(Victim);
        GetWorldTimerManager().SetTimer(Respawn, FTimerDelegate::CreateWeakLambda(this, [this,WeakPC,WeakPawn]() {
            if (WeakPC.IsValid()) { WeakPC->UnPossess(); RestartPlayer(WeakPC.Get()); SpawnSupplies(ArenaOrigin); }
            if (WeakPawn.IsValid()) WeakPawn->Destroy();
        }), 5.f, false);
    }
}
void ACityPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &ACityPlayerController::HostCity);
    InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &ACityPlayerController::OpenArena);
    InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &ACityPlayerController::OpenParkourCity);
}
void ACityPlayerController::BeginPlay()
{
    Super::BeginPlay();
#if !UE_BUILD_SHIPPING
    auto Schedule = [this](float Delay, TFunction<void()> Action) {
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, MoveTemp(Action)), Delay, false);
    };
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityPickupTest")))
    {
        Schedule(1.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->SetActorLocation(FVector(-3088,468,130)); });
        Schedule(1.4f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->Interact(); });
        Schedule(1.8f,[this]() { auto* P=Cast<ACityCharacter>(GetPawn()); UE_LOG(LogTemp,Display,TEXT("CITY_START_ONE_PICKUP %s"),P && P->bEquipped?TEXT("PASS"):TEXT("FAIL")); });
        Schedule(2.1f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->bEquipped=false; P->Ammo=0; P->ReserveAmmo=0; P->SetActorLocation(FVector(-3088,668,130)); } });
        Schedule(2.5f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->Interact(); });
        Schedule(3.f,[this]() { auto* P=Cast<ACityCharacter>(GetPawn()); UE_LOG(LogTemp,Display,TEXT("CITY_START_TWO_PICKUP %s"),P && P->bEquipped?TEXT("PASS"):TEXT("FAIL")); });
        Schedule(3.5f,[]() { FPlatformMisc::RequestExit(false); });
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityPitTest")))
    {
        Schedule(1.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->Health=10000; P->SetActorLocation(FVector(9600,-3150,0)); SetControlRotation(FRotator(0,90,0)); } });
        Schedule(4.f,[this]() { auto* P=Cast<ACityCharacter>(GetPawn()); const FVector L=P?P->GetActorLocation():FVector::ZeroVector; UE_LOG(LogTemp,Display,TEXT("CITY_PIT_STAIRS %s location=%s"),P && FMath::Abs(L.X-9600)<100 && L.Y>-2720 && L.Z>220 && P->GetCharacterMovement()->IsMovingOnGround()?TEXT("PASS"):TEXT("FAIL"),*L.ToString()); });
        Schedule(4.1f,[this]() { SetControlRotation(FRotator(0,-153.435f,0)); });
        Schedule(6.8f,[this]() { auto* P=Cast<ACityCharacter>(GetPawn()); const FVector L=P?P->GetActorLocation():FVector::ZeroVector; UE_LOG(LogTemp,Display,TEXT("CITY_PIT_STREET_EXIT %s location=%s"),P && L.X<9200 && L.X>8800 && L.Y<-2900 && L.Z>100 && L.Z<220 && P->GetCharacterMovement()->IsMovingOnGround()?TEXT("PASS"):TEXT("FAIL"),*L.ToString()); });
        Schedule(7.5f,[]() { FPlatformMisc::RequestExit(false); });
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityBattleTest")))
    {
        Schedule(.5f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->Health=10000; P->GiveRifle(); } });
        Schedule(1.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) UE_LOG(LogTemp,Display,TEXT("CITY_BATTLE_ASSETS %s sounds=%d"),P->FireSounds.Num()==3 && P->ReloadSound && P->MuzzleFX && P->ImpactFX && P->FireAnimation && P->MovingFireAnimation && P->MovingReloadAnimation && P->JumpStartAnimation && P->LandAnimation?TEXT("PASS"):TEXT("FAIL"),P->FireSounds.Num()); });
        Schedule(8.f,[this]() { auto* State=GetWorld()->GetGameState<ACityGameState>(); UE_LOG(LogTemp,Display,TEXT("CITY_BATTLE_BOTS %s alive=%d"),State && State->BotsAlive>=3?TEXT("PASS"):TEXT("FAIL"),State?State->BotsAlive:-1); });
        Schedule(9.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->AimPressed(); P->StartFire(); } });
        Schedule(9.4f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->StopFire(); P->Reload(); } });
        Schedule(11.5f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) UE_LOG(LogTemp,Display,TEXT("CITY_BATTLE_AMMO %s ammo=%d reserve=%d"),P->Ammo==30 && P->ReserveAmmo<90 && !P->bReloading?TEXT("PASS"):TEXT("FAIL"),P->Ammo,P->ReserveAmmo); });
        Schedule(12.f,[this]() {
            auto* P=Cast<ACityCharacter>(GetPawn());
            for (TActorIterator<ACityCharacter> It(GetWorld());It;++It) if (It->bIsBot && It->IsAlive()) UGameplayStatics::ApplyDamage(*It,1000.f,this,P,UDamageType::StaticClass());
            auto* State=GetWorld()->GetGameState<ACityGameState>();
            UE_LOG(LogTemp,Display,TEXT("CITY_BATTLE_ROUND %s"),State && State->RoundsWon==1?TEXT("PASS"):TEXT("FAIL"));
            if (P) { P->AimReleased(); P->SetActorLocation(FVector(9667,-2960,-90)); }
        });
        Schedule(12.3f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->ServerRecover(); });
        Schedule(12.8f,[this]() { auto* P=Cast<ACityCharacter>(GetPawn()); auto* GM=GetWorld()->GetAuthGameMode<ACityGameMode>(); UE_LOG(LogTemp,Display,TEXT("CITY_BATTLE_RECOVERY %s"),P && GM && FVector::Dist2D(P->GetActorLocation(),GM->ArenaOrigin)<500?TEXT("PASS"):TEXT("FAIL")); });
        Schedule(14.f,[]() { FPlatformMisc::RequestExit(false); });
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityConnectivityTest")))
    {
        Schedule(1.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->SetActorLocation(FVector(-6500,568,890.15f)); SetControlRotation(FRotator::ZeroRotator); P->GetCharacterMovement()->MaxWalkSpeed=550.f; } });
        Schedule(9.2f,[this]() {
            auto* P=Cast<ACityCharacter>(GetPawn());
            UE_LOG(LogTemp,Display,TEXT("CITY_STREET_CONNECTION %s location=%s"),P && P->IsAlive() && P->GetActorLocation().X>-3200 && P->GetActorLocation().Z>75 && P->GetCharacterMovement()->IsMovingOnGround()?TEXT("PASS"):TEXT("FAIL"),P?*P->GetActorLocation().ToString():TEXT("none"));
            if (P)
            {
                FVector Landing; float Top; FHitResult Hit;
                GetWorld()->LineTraceSingleByChannel(Hit,P->GetActorLocation(),P->GetActorLocation()+FVector(250,0,0),ECC_WorldStatic,FCollisionQueryParams(SCENE_QUERY_STAT(ConnectionCheck),false,P));
                UE_LOG(LogTemp,Display,TEXT("CITY_STREET_OBSTACLE %s vault=%d"),*GetNameSafe(Hit.GetActor()),P->FindVault(Landing,Top));
            }
        });
        Schedule(10.f,[]() { FPlatformMisc::RequestExit(false); });
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityMapTravelTest")))
    {
        static int32 Stage=0;
        const bool City=UGameplayStatics::GetCurrentLevelName(this,true).Contains(TEXT("CityOpen"));
        if (Stage==0 && !City) { ++Stage; Schedule(2.f,[this]() { OpenParkourCity(); }); }
        else if (Stage==1 && City) { ++Stage; UE_LOG(LogTemp,Display,TEXT("CITY_MAP_TRAVEL CITY PASS")); Schedule(2.f,[this]() { OpenArena(); }); }
        else if (Stage==2 && !City) { ++Stage; UE_LOG(LogTemp,Display,TEXT("CITY_MAP_TRAVEL ARENA PASS")); Schedule(2.f,[]() { FPlatformMisc::RequestExit(false); }); }
        else { UE_LOG(LogTemp,Error,TEXT("CITY_MAP_TRAVEL FAIL stage=%d"),Stage); Schedule(2.f,[]() { FPlatformMisc::RequestExit(false); }); }
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityParkourTest")))
    {
        const bool City=UGameplayStatics::GetCurrentLevelName(this,true).Contains(TEXT("CityOpen"));
        const FVector Start=City?FVector(-9100,600,890.15f):FVector(-2500,2000,90.15f);
        Schedule(1.f,[this,Start]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->SetActorLocation(Start); SetControlRotation(FRotator::ZeroRotator); } });
        Schedule(2.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { FVector L; float H; UE_LOG(LogTemp,Display,TEXT("CITY_PARKOUR_DETECT %s"),P->FindVault(L,H)?TEXT("PASS"):TEXT("FAIL")); P->JumpOrVault(); } });
        Schedule(3.2f,[this,Start]() { auto* P=Cast<ACityCharacter>(GetPawn()); UE_LOG(LogTemp,Display,TEXT("CITY_PARKOUR_VAULT %s x=%g"),P && P->GetActorLocation().X>Start.X+150 && !P->bVaulting?TEXT("PASS"):TEXT("FAIL"),P?P->GetActorLocation().X:0.f); });
        Schedule(3.5f,[this,City]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->SetActorLocation(City?FVector(-7110,1800,890.15f):FVector(2960,2400,90.15f)); } });
        Schedule(4.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { FVector L; float H; UE_LOG(LogTemp,Display,TEXT("CITY_PARKOUR_HEIGHT_LIMIT %s"),!P->FindVault(L,H)?TEXT("PASS"):TEXT("FAIL")); P->GiveRifle(); P->AimPressed(); } });
        Schedule(5.f,[this,City]() {
            if (auto* P=Cast<ACityCharacter>(GetPawn()))
            {
                UE_LOG(LogTemp,Display,TEXT("CITY_SCOPE_4X %s fov=%g"),P->IsScoped() && P->Camera->FieldOfView<29?TEXT("PASS"):TEXT("FAIL"),P->Camera->FieldOfView);
                P->SetActorLocation(City?FVector(-9500,600,890.15f):FVector(-2600,2000,90.15f));
                if (auto* Target=GetWorld()->SpawnActor<ACityCharacter>(P->GetActorLocation()+FVector(1000,0,0),FRotator::ZeroRotator))
                {
                    Target->bIsBot=true;
                    P->StartFire(); P->StopFire();
                    UE_LOG(LogTemp,Display,TEXT("CITY_SCOPE_HIT %s health=%g"),Target->Health==75.f?TEXT("PASS"):TEXT("FAIL"),Target->Health);
                }
                P->Ammo=20; P->Reload();
                UE_LOG(LogTemp,Display,TEXT("CITY_SCOPE_RELOAD %s"),!P->IsScoped()?TEXT("PASS"):TEXT("FAIL"));
            }
        });
        Schedule(8.f,[]() { FPlatformMisc::RequestExit(false); });
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityVisualTest")))
    {
        Schedule(2.f,[this]() {
            if (auto* P=Cast<ACityCharacter>(GetPawn()))
            {
                UE_LOG(LogTemp,Display,TEXT("CITY_VISUAL player=%s pickup=%s"),*P->GetActorLocation().ToString(),P->FindPickup()?TEXT("nearby"):TEXT("none"));
                for (TActorIterator<ACityPickup> It(GetWorld());It;++It) UE_LOG(LogTemp,Display,TEXT("CITY_VISUAL rifle=%s"),*It->GetActorLocation().ToString());
            }
        });
        Schedule(8.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->Interact(); });
        Schedule(15.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->AimPressed(); });
        Schedule(75.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->AimReleased(); P->StartFire(); } });
        Schedule(76.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->StopFire(); P->Reload(); } });
    }
    if (IsLocalController() && GetNetMode() == NM_Client && FParse::Param(FCommandLine::Get(), TEXT("CityNetTest")))
    {
        Schedule(3.f, [this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->Interact(); });
        Schedule(4.f, [this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->StartFire(); });
        Schedule(4.6f, [this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { P->StopFire(); P->Reload(); } });
        if (FParse::Param(FCommandLine::Get(),TEXT("CityVaultNetTest")))
        {
            Schedule(6.8f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { FVector L; float H; UE_LOG(LogTemp,Display,TEXT("CITY_NET_VAULT_DETECT %s location=%s"),P->FindVault(L,H)?TEXT("PASS"):TEXT("FAIL"),*P->GetActorLocation().ToString()); P->JumpOrVault(); } });
            Schedule(7.6f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->AimPressed(); });
        }
        Schedule(8.f, [this]() {
            ACityCharacter* P = Cast<ACityCharacter>(GetPawn());
            int32 Humans=0;
            for (TActorIterator<ACityCharacter> It(GetWorld()); It; ++It) if (!It->bIsBot) ++Humans;
            const bool VaultPassed=!FParse::Param(FCommandLine::Get(),TEXT("CityVaultNetTest")) || (P && P->GetActorLocation().X>-2300 && !P->bVaulting && P->IsScoped());
            const bool Passed = P && P->bEquipped && P->Ammo==30 && P->ReserveAmmo<90 && !P->bReloading && Humans>=2 && VaultPassed;
            if (FParse::Param(FCommandLine::Get(),TEXT("CityVaultNetTest"))) UE_LOG(LogTemp,Display,TEXT("CITY_NET_VAULT_SCOPE %s x=%g scoped=%d"),VaultPassed?TEXT("PASS"):TEXT("FAIL"),P?P->GetActorLocation().X:0.f,P?P->IsScoped():0);
            UE_LOG(LogTemp, Display, TEXT("CITY_NET_CLIENT %s humans=%d equipped=%d ammo=%d reserve=%d reloading=%d"),
                Passed ? TEXT("PASS") : TEXT("FAIL"), Humans,P?P->bEquipped:0,P?P->Ammo:-1,P?P->ReserveAmmo:-1,P?P->bReloading:0);
            ServerNetTestReport();
        });
        Schedule(9.5f, []() { FPlatformMisc::RequestExit(false); });
    }
    if (IsLocalController() && HasAuthority() && FParse::Param(FCommandLine::Get(),TEXT("CityArenaTest")))
    {
        Schedule(0.5f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->Health=10000.f; });
        Schedule(2.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) P->Interact(); });
        Schedule(3.f,[this]() { if (auto* P=Cast<ACityCharacter>(GetPawn())) { UE_LOG(LogTemp,Display,TEXT("CITY_ARENA_PICKUP %s"),P->bEquipped?TEXT("PASS"):TEXT("FAIL")); } });
        Schedule(2.5f,[this]() {
            if (auto* P=Cast<ACityCharacter>(GetPawn()))
            {
                P->SetActorLocation(FVector(-2000,-900,100));
            }
        });
        Schedule(4.6f,[this]() {
            auto* P=Cast<ACityCharacter>(GetPawn());
            FVector L=P?P->GetActorLocation():FVector::ZeroVector;
            UE_LOG(LogTemp,Display,TEXT("CITY_ARENA_STEP %s x=%g z=%g"),L.X>-1700&&L.Z>110?TEXT("PASS"):TEXT("FAIL"),L.X,L.Z);
        });
        Schedule(10.f,[this]() {
            auto* P=Cast<ACityCharacter>(GetPawn());
            for (TActorIterator<ACityCharacter> It(GetWorld());It;++It) if (It->bIsBot && It->IsAlive()) UGameplayStatics::ApplyDamage(*It,500.f,this,P,UDamageType::StaticClass());
            auto* State=GetWorld()->GetGameState<ACityGameState>();
            UE_LOG(LogTemp,Display,TEXT("CITY_ARENA_ROUND %s won=%d"),State&&State->RoundsWon==1?TEXT("PASS"):TEXT("FAIL"),State?State->RoundsWon:-1);
        });
        Schedule(11.f,[this]() { if (APawn* P=GetPawn()) P->SetActorLocation(FVector(6500,0,150)); });
        Schedule(14.f,[this]() { auto* P=Cast<ACityCharacter>(GetPawn()); UE_LOG(LogTemp,Display,TEXT("CITY_ARENA_FALL %s"),P&&P->bDead?TEXT("PASS"):TEXT("FAIL")); });
        Schedule(15.f,[]() { FPlatformMisc::RequestExit(false); });
    }
#endif
}
void ACityPlayerController::ServerNetTestReport_Implementation()
{
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("CityNetTest")))
    {
        ACityCharacter* P = Cast<ACityCharacter>(GetPawn());
        const bool VaultPassed=!FParse::Param(FCommandLine::Get(),TEXT("CityVaultNetTest")) || (P && P->GetActorLocation().X>-2300 && !P->bVaulting && P->IsScoped());
        UE_LOG(LogTemp, Display, TEXT("CITY_NET_SERVER %s ammo=%d reserve=%d"),
            P && P->bEquipped && P->Ammo==30 && P->ReserveAmmo<90 && VaultPassed ? TEXT("PASS") : TEXT("FAIL"),P?P->Ammo:-1,P?P->ReserveAmmo:-1);
    }
#endif
}
void ACityPlayerController::HostCity()
{
    if (GetNetMode() == NM_Client) return;
    const bool City=UGameplayStatics::GetCurrentLevelName(this,true).Contains(TEXT("CityOpen"));
    GetWorld()->ServerTravel(City?TEXT("/Game/SoulCityGame/Maps/L_CityOpen?listen"):TEXT("/Game/SoulCityGame/Maps/L_CityCombat?listen"));
}
void ACityPlayerController::TravelToMap(const FString& MapPath)
{
    if (!HasAuthority()) return;
    if (GetNetMode()==NM_Standalone) UGameplayStatics::OpenLevel(this,FName(*MapPath));
    else GetWorld()->ServerTravel(MapPath+TEXT("?listen"));
}
void ACityPlayerController::OpenArena() { TravelToMap(TEXT("/Game/SoulCityGame/Maps/L_CityCombat")); }
void ACityPlayerController::OpenParkourCity() { TravelToMap(TEXT("/Game/SoulCityGame/Maps/L_CityOpen")); }
void ACityPlayerController::JoinCity(const FString& Address)
{
    FString Clean = Address.TrimStartAndEnd();
    if (!Clean.IsEmpty() && !Clean.Contains(TEXT(" ")) && !Clean.Contains(TEXT("?"))) ClientTravel(Clean, TRAVEL_Absolute);
}
void ACityPlayerController::CityRestart() { if (HasAuthority()) RestartLevel(); }

void ACityHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas) return;
    auto* Player = Cast<ACityCharacter>(GetOwningPawn());
    if (!Player) return;
    const float X = Canvas->ClipX, Y = Canvas->ClipY;
    if (Player->IsScoped())
    {
        const float CX=X*0.5f,CY=Y*0.5f,R=FMath::Min(X,Y)*0.43f;
        const FLinearColor Black(0.004f,0.007f,0.012f,1.f);
        DrawRect(Black,0,0,X,CY-R); DrawRect(Black,0,CY+R,X,Y-CY-R);
        for (int32 I=0;I<128;++I)
        {
            const float DY=-R+(I+0.5f)*2*R/128.f;
            const float Span=FMath::Sqrt(FMath::Max(0.f,R*R-DY*DY));
            const float PY=CY-R+I*2*R/128.f;
            DrawRect(Black,0,PY,CX-Span,2*R/128.f+1.f);
            DrawRect(Black,CX+Span,PY,X-CX-Span,2*R/128.f+1.f);
        }
        for (int32 I=0;I<96;++I)
        {
            const float A=I*2*PI/96.f,B=(I+1)*2*PI/96.f;
            DrawLine(CX+FMath::Cos(A)*(R-7),CY+FMath::Sin(A)*(R-7),CX+FMath::Cos(B)*(R-7),CY+FMath::Sin(B)*(R-7),FColor(30,45,53),12.f);
            DrawLine(CX+FMath::Cos(A)*(R-15),CY+FMath::Sin(A)*(R-15),CX+FMath::Cos(B)*(R-15),CY+FMath::Sin(B)*(R-15),FColor(80,160,170,140),1.f);
        }
        const FColor Ink(15,22,25);
        DrawLine(CX-R+28,CY,CX-12,CY,Ink,1.4f); DrawLine(CX+12,CY,CX+R-28,CY,Ink,1.4f);
        DrawLine(CX,CY-R+28,CX,CY-12,Ink,1.4f); DrawLine(CX,CY+12,CX,CY+R-28,Ink,1.4f);
        for (int32 I=1;I<=4;++I)
        {
            const float D=I*R*0.14f;
            DrawLine(CX-5,CY+D,CX+5,CY+D,Ink,1.5f);
            DrawLine(CX-5,CY-D,CX+5,CY-D,Ink,1.5f);
            DrawLine(CX+D,CY-4,CX+D,CY+4,Ink,1.5f);
            DrawLine(CX-D,CY-4,CX-D,CY+4,Ink,1.5f);
            DrawText(FString::FromInt(I*2),Ink,CX+11,CY+D-6,nullptr,0.65f);
        }
        DrawRect(FLinearColor(1.f,0.15f,0.08f),CX-1.5f,CY-1.5f,3,3);
        DrawText(TEXT("MURDOCK // PRECISION OPTIC"),FColor(130,200,200),CX-122,CY+R-50,nullptr,0.8f);
        DrawText(TEXT("4X"),FColor::White,CX-10,CY-R+37,nullptr,1.1f);
    }
    DrawRect(FLinearColor(0.015f,0.025f,0.045f,0.8f), 22, Y-122, 385, 95);
    DrawText(FString::Printf(TEXT("HEALTH  %03.0f"), Player->Health), FColor::White, 38, Y-107, nullptr, 1.5f);
    DrawRect(FLinearColor(0.08f,0.12f,0.16f), 38, Y-72, 180, 8);
    DrawRect(FLinearColor(0.15f,0.8f,0.7f), 38, Y-72, 180 * Player->Health/100.f, 8);
    const FString Weapon = !Player->bEquipped ? TEXT("NO WEAPON - FIND A RIFLE") : Player->bReloading ? TEXT("RELOADING...") : FString::Printf(TEXT("RIFLE  %02d / %03d"), Player->Ammo, Player->ReserveAmmo);
    DrawText(Weapon, FColor::Cyan, 38, Y-53, nullptr, 1.1f);
    if (Player->IsAlive() && !Player->IsScoped())
    {
        const FColor Color=Player->bEquipped ? FColor(230,250,255) : FColor(140,160,170);
        const float Gap=Player->IsAiming() ? 3.f : 7.f;
        DrawLine(X/2-Gap-10,Y/2,X/2-Gap,Y/2,Color,2.f); DrawLine(X/2+Gap,Y/2,X/2+Gap+10,Y/2,Color,2.f);
        DrawLine(X/2,Y/2-Gap-10,X/2,Y/2-Gap,Color,2.f); DrawLine(X/2,Y/2+Gap,X/2,Y/2+Gap+10,Color,2.f);
        DrawRect(FLinearColor::White,X/2-1,Y/2-1,2,2);
    }
    if (Player->FindPickup() && !Player->IsScoped())
    {
        const float PX=X/2-165, PY=Y-155;
        DrawRect(FLinearColor(0.02f,0.045f,0.065f,0.94f),PX,PY,330,76);
        DrawRect(FLinearColor(0.15f,0.8f,0.9f),PX,PY,4,76);
        DrawRect(FLinearColor(0.12f,0.25f,0.32f),PX+17,PY+16,42,42);
        DrawText(TEXT("E"),FColor::White,PX+29,PY+22,nullptr,1.7f);
        DrawText(Player->bEquipped ? TEXT("YEDEK MERMI") : TEXT("MURDOCK TUFEGI"),FColor::White,PX+75,PY+15,nullptr,1.1f);
        DrawText(Player->bEquipped ? TEXT("Almak icin E'ye bas") : TEXT("Silahi almak icin E'ye bas"),FColor(155,205,220),PX+75,PY+42,nullptr,0.9f);
    }
    else
    {
        FVector Landing; float Top;
        if (!Player->IsScoped() && Player->FindVault(Landing,Top))
        {
            DrawRect(FLinearColor(0.02f,0.045f,0.065f,0.94f),X/2-140,Y-145,280,48);
            DrawText(TEXT("SPACE  |  ENGELIN UZERINDEN AS"),FColor(120,235,220),X/2-124,Y-130,nullptr,0.95f);
        }
    }
    if (!Player->IsAlive()) DrawText(TEXT("DOWN - RESPAWNING IN 5 SECONDS"), FColor::Red,X/2-190,Y/2-55,nullptr,1.4f);
    if (auto* State = GetWorld()->GetGameState<ACityGameState>())
    {
        DrawRect(FLinearColor(0.02f,0.035f,0.055f,0.9f),X/2-235,20,470,68);
        DrawText(State->bCombatMode?FString::Printf(TEXT("KAZANILAN TUR  %02d     |     TUR  %02d"),State->RoundsWon,State->Wave):TEXT("SOUL CITY  //  PARKUR VE SERBEST GEZI"),FColor::White,X/2-210,31,nullptr,1.25f);
        const int32 Wait=FMath::Max(0,FMath::CeilToInt(State->NextWaveTime-State->GetServerWorldTimeSeconds()));
        const FString Status=!State->bCombatMode?TEXT("F6 ILE SEHIR ARENASINA GEC"):State->BotsAlive>0 ? FString::Printf(TEXT("KALAN BOT: %d      TAKIM TOPLAM: %d"),State->BotsAlive,State->TeamKills) : FString::Printf(TEXT("YENI TUR %d SANIYE SONRA"),Wait);
        DrawText(Status,FColor(80,220,235),X/2-170,62,nullptr,0.95f);
        const float TX=X-262;
        DrawRect(FLinearColor(0.02f,0.035f,0.055f,0.9f),TX,110,242,40+State->PlayerArray.Num()*53);
        DrawText(TEXT("TAKIM    OLDURME / OLUM"),FColor(125,210,225),TX+12,121,nullptr,0.9f);
        int32 Row=0;
        for (APlayerState* BasePS : State->PlayerArray)
        {
            auto* PS=Cast<ACityPlayerState>(BasePS);
            auto* Mate=BasePS ? Cast<ACityCharacter>(BasePS->GetPawn()) : nullptr;
            if (!PS) continue;
            const float RY=151+Row*53;
            const bool Self=PS==Player->GetPlayerState();
            DrawText(FString::Printf(TEXT("%s%s   %d / %d"),*PS->GetPlayerName(),Self?TEXT(" (Sen)"):TEXT(""),PS->Kills,PS->Deaths),FColor::White,TX+12,RY,nullptr,0.85f);
            DrawRect(FLinearColor(0.1f,0.15f,0.2f),TX+12,RY+24,210,5);
            DrawRect(FLinearColor(0.15f,0.75f,0.65f),TX+12,RY+24,210*(Mate?Mate->Health:0)/100.f,5);
            ++Row;
        }
    }
    for (TActorIterator<ACityCharacter> It(GetWorld()); It; ++It)
    {
        if (!It->bIsBot || !It->IsAlive() || !It->WasRecentlyRendered(0.1f)) continue;
        const FVector Position = It->GetActorLocation()+FVector(0,0,120);
        FVector Screen = Project(Position);
        if (Screen.Z <= 0) continue;
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(EnemyLabel),false,Player);
        if (GetWorld()->LineTraceSingleByChannel(Hit,Player->Camera->GetComponentLocation(),Position,ECC_WorldStatic,Params)) continue;
        DrawText(TEXT("HOSTILE"),FColor(255,90,70),Screen.X-28,Screen.Y-15,nullptr,0.8f);
        DrawRect(FLinearColor(0.15f,0.03f,0.03f),Screen.X-28,Screen.Y,56,4);
        DrawRect(FLinearColor(0.9f,0.12f,0.08f),Screen.X-28,Screen.Y,56*It->Health/100.f,4);
    }
    ACityCharacter* NearestBot=nullptr; float NearestDistance=FLT_MAX;
    for (TActorIterator<ACityCharacter> It(GetWorld());It;++It)
        if (It->bIsBot && It->IsAlive()) { const float D=FVector::DistSquared(Player->GetActorLocation(),It->GetActorLocation()); if (D<NearestDistance) { NearestDistance=D; NearestBot=*It; } }
    if (NearestBot)
    {
        const bool Ahead=FVector::DotProduct(Player->GetActorForwardVector(),NearestBot->GetActorLocation()-Player->GetActorLocation())>0;
        DrawText(FString::Printf(TEXT("BOT  %.0f m  /  %s"),FMath::Sqrt(NearestDistance)/100.f,Ahead?TEXT("ILERIDE"):TEXT("ARKANDA")),FColor(255,125,85),X/2-110,Player->IsScoped()?130.f:102.f,nullptr,1.1f);
        FVector Position=Project(NearestBot->GetActorLocation()+FVector(0,0,130));
        if (Position.Z>0)
        {
            const float BX=FMath::Clamp(Position.X,40.f,X-40.f),BY=FMath::Clamp(Position.Y,150.f,Y-190.f);
            DrawLine(BX-7,BY-7,BX,BY,FColor(255,100,65),2.f); DrawLine(BX+7,BY-7,BX,BY,FColor(255,100,65),2.f);
        }
    }
    DrawRect(FLinearColor(0.02f,0.035f,0.055f,0.85f),22,103,345,53);
    DrawRect(FLinearColor(0.02f,0.035f,0.055f,0.85f),22,20,345,65);
    DrawText(UGameplayStatics::GetCurrentLevelName(this,true).Contains(TEXT("CityOpen"))?TEXT("SOUL CITY  /  SERBEST BOLGE"):TEXT("SOUL CITY  /  SEHIR ARENASI"),FColor(110,230,215),34,32,nullptr,1.05f);
    DrawText(TEXT("F6 SEHIR SAVASI   F7 PARKUR   H KURTUL"),FColor(205,220,230),34,60,nullptr,0.85f);
    DrawText(TEXT("E SILAH AL   R SARJOR   SAG TIK 4X   SOL TIK ATES"),FColor(190,205,220),30,110,nullptr,0.85f);
    DrawText(GetNetMode()==NM_Standalone ? TEXT("F5: LAN ODASI AC  |  JoinCity <IP>:7777") : TEXT("CO-OP - TAKIM ARKADASINA HASAR YOK"), FColor(150,180,205),30,132,nullptr,0.8f);
}
