#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "CityCombatActors.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UBlendSpace;
class UAnimSequence;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class USceneComponent;
class USoundBase;
class USoundAttenuation;
class UParticleSystem;
class ACityPickup;

UCLASS()
class CITYCOMBAT_API ACityCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ACityCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    virtual float TakeDamage(float Amount, const FDamageEvent& Event, AController* InstigatorController, AActor* Causer) override;
    virtual void FellOutOfWorld(const class UDamageType& DamageType) override;

    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> FillLight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> OpticRoot;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UBlendSpace> Locomotion;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> JumpAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> ReloadAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> DeathAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> RifleReadyAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> FireAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> MovingFireAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> MovingReloadAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> JumpStartAnimation;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UAnimSequence> LandAnimation;
    UPROPERTY(EditDefaultsOnly) TArray<TObjectPtr<USoundBase>> FireSounds;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<USoundBase> ReloadSound;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<USoundBase> FootstepSound;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UParticleSystem> MuzzleFX;
    UPROPERTY(EditDefaultsOnly) TObjectPtr<UParticleSystem> ImpactFX;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USoundAttenuation> WeaponAttenuation;
    UPROPERTY(ReplicatedUsing=OnRep_Combat, BlueprintReadOnly) float Health = 100.f;
    UPROPERTY(ReplicatedUsing=OnRep_Combat, BlueprintReadOnly) int32 Ammo = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 ReserveAmmo = 0;
    UPROPERTY(ReplicatedUsing=OnRep_Combat, BlueprintReadOnly) bool bEquipped = false;
    UPROPERTY(ReplicatedUsing=OnRep_Combat, BlueprintReadOnly) bool bReloading = false;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bIsBot = false;
    UPROPERTY(ReplicatedUsing=OnRep_Sprint) bool bSprinting = false;
    UPROPERTY(ReplicatedUsing=OnRep_Combat, BlueprintReadOnly) bool bDead = false;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bVaulting = false;
    UPROPERTY(EditDefaultsOnly) int32 MagazineSize = 30;
    UPROPERTY(EditDefaultsOnly) float ReloadSeconds = 1.8f;
    UPROPERTY(EditDefaultsOnly) float FireInterval = 0.13f;
    UPROPERTY(EditDefaultsOnly) float WalkSpeed = 300.f;
    UPROPERTY(EditDefaultsOnly) float SprintSpeed = 550.f;

    UFUNCTION(BlueprintCallable) void StartFire();
    UFUNCTION(BlueprintCallable) void StopFire();
    UFUNCTION(BlueprintCallable) void Reload();
    UFUNCTION(BlueprintCallable) void Interact();
    UFUNCTION(BlueprintCallable) void AimPressed();
    UFUNCTION(BlueprintCallable) void AimReleased();
    void ToggleAim();
    UFUNCTION(BlueprintPure) bool IsAlive() const { return !bDead && Health > 0.f; }
    bool IsAiming() const { return bAiming; }
    bool IsScoped() const { return bAiming && bEquipped && IsAlive() && !bReloading && !bVaulting; }
    bool FindVault(FVector& Landing, float& Top) const;
    void JumpOrVault();
    UFUNCTION(Server, Reliable) void ServerRecover();
    UFUNCTION(BlueprintPure) ACityPickup* FindPickup() const;
    void GiveRifle(int32 SpareAmmo = 90);
    void BotThink();

    UFUNCTION(Server, Reliable) void ServerStartFire();
    UFUNCTION(Server, Reliable) void ServerStopFire();
    UFUNCTION(Server, Reliable) void ServerReload();
    UFUNCTION(Server, Reliable) void ServerInteract();
    UFUNCTION(Server, Reliable) void ServerSprint(bool bEnabled);
    UFUNCTION(Server, Reliable) void ServerAim(bool bEnabled);
    UFUNCTION(Server, Reliable) void ServerTryVault();
    UFUNCTION(NetMulticast, Reliable) void MulticastVault(FVector Start, FVector Target, float Top, float ServerTime);
    UFUNCTION(NetMulticast, Reliable) void MulticastEndVault(FVector Location);
    UFUNCTION(NetMulticast, Reliable) void MulticastReloadAudio();
    UFUNCTION(NetMulticast, Unreliable) void MulticastShot(FVector_NetQuantize Start, FVector_NetQuantize End, bool bHit);
protected:
    UFUNCTION() void OnRep_Combat();
    UFUNCTION() void OnRep_Sprint();
    void MoveForward(float Axis);
    void MoveRight(float Axis);
    void Turn(float Axis);
    void LookUp(float Axis);
    void SprintPressed();
    void SprintReleased();
    void FireShot();
    void FinishReload();
    void UpdateVisuals(float DeltaTime);
    void SetVisualAnimation(class UAnimationAsset* Asset, bool bLooping, float Rate = 1.f);
    void Die(AController* Killer);
    FTimerHandle FireTimer, ReloadTimer, BotTimer;
    double LastShotTime = -100.;
    double SpawnTime = 0.;
    float BotRetargetTime = 0.f;
    TWeakObjectPtr<ACityCharacter> BotTarget;
    UPROPERTY(Replicated) bool bAiming = false;
    FVector VaultStart=FVector::ZeroVector, VaultTarget=FVector::ZeroVector;
    float VaultTop=0.f, VaultStartTime=0.f;
    float VisualFireUntil=0.f, JumpStartUntil=0.f, LandUntil=0.f, NextFootstep=0.f;
    FVector LastSafeLocation=FVector::ZeroVector;
    float NextSafetyCheck=0.f, DisconnectedSince=0.f, LastRecoveryTime=-100.f;
    bool bVisualWasFalling=false;
    bool bUsingNavigation = false;
    bool bVisualDead = false;
    bool bVisualEquipped = false;
    UPROPERTY(Transient) TObjectPtr<class UAnimationAsset> VisualAsset;
    TArray<FName> WeaponBones;
};

UCLASS()
class CITYCOMBAT_API ACityPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Kills=0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Deaths=0;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};

UCLASS()
class CITYCOMBAT_API ACityPickup : public AActor
{
    GENERATED_BODY()
public:
    ACityPickup();
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Case;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Barrel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
    bool TryCollect(ACityCharacter* Player);
    bool bCollected = false;
};

UCLASS()
class CITYCOMBAT_API ACityBulletVisual : public AActor
{
    GENERATED_BODY()
public:
    ACityBulletVisual();
    virtual void Tick(float DeltaSeconds) override;
    void Configure(FVector Start,FVector End,UParticleSystem* Impact,bool bEnemy);
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(Transient) TObjectPtr<UParticleSystem> ImpactTemplate;
private:
    FVector Origin=FVector::ZeroVector,Direction=FVector::ForwardVector;
    float Length=0.f,Travel=0.f;
};

UCLASS()
class CITYCOMBAT_API ACityGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    UPROPERTY(Replicated, BlueprintReadOnly) int32 Wave = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 BotsAlive = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 TeamKills = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) int32 RoundsWon = 0;
    UPROPERTY(Replicated, BlueprintReadOnly) float NextWaveTime = 0.f;
    UPROPERTY(Replicated, BlueprintReadOnly) bool bCombatMode=true;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
};

UCLASS()
class CITYCOMBAT_API ACityGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACityGameMode();
    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* Player) override;
    void CharacterDied(ACityCharacter* Victim, AController* Killer);
    void StartWave();
    bool FindFloor(const FVector& Wanted, FVector& OutLocation) const;
    void SpawnSupplies(const FVector& Around);
    FVector ArenaOrigin;
protected:
    FTimerHandle WaveTimer;
};

UCLASS()
class CITYCOMBAT_API ACityPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    UFUNCTION(Exec) void HostCity();
    UFUNCTION(Exec) void JoinCity(const FString& Address);
    UFUNCTION(Exec) void CityRestart();
    UFUNCTION(Exec) void OpenArena();
    UFUNCTION(Exec) void OpenParkourCity();
    void TravelToMap(const FString& MapPath);
    UFUNCTION(Server, Reliable) void ServerNetTestReport();
};

UCLASS()
class CITYCOMBAT_API ACityHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
