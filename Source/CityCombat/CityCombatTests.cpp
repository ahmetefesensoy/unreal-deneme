#include "CityCombatActors.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCityCombatRules, "CityCombat.Gameplay.AuthorityRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCityCombatRules::RunTest(const FString&)
{
    const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    World->InitializeActorsForPlay(FURL());
    World->BeginPlay();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ACityCharacter* Player = World->SpawnActor<ACityCharacter>(FVector(0,0,100), FRotator::ZeroRotator, Spawn);
    ACityCharacter* Friend = World->SpawnActor<ACityCharacter>(FVector(800,0,100), FRotator::ZeroRotator, Spawn);
    ACityCharacter* Enemy = World->SpawnActor<ACityCharacter>(FVector(1000,0,100), FRotator::ZeroRotator, Spawn);
    Enemy->bIsBot = true;
    ACityPickup* Pickup = World->SpawnActor<ACityPickup>(FVector(50,0,100), FRotator::ZeroRotator, Spawn);
    TestFalse(TEXT("Player starts unarmed"), Player->bEquipped);
    TestTrue(TEXT("Nearby rifle can be picked up"), Pickup->TryCollect(Player));
    TestEqual(TEXT("Pickup fills magazine"), Player->Ammo, 30);
    TestEqual(TEXT("Pickup supplies reserve"), Player->ReserveAmmo, 90);
    TestFalse(TEXT("Same rifle cannot be collected twice"), Pickup->TryCollect(Friend));
    ACityPickup* FarPickup = World->SpawnActor<ACityPickup>(FVector(10000,0,100), FRotator::ZeroRotator, Spawn);
    TestFalse(TEXT("Cannot collect from a distance"), FarPickup->TryCollect(Player));
    Player->StartFire(); Player->StopFire();
    TestEqual(TEXT("One shot consumes exactly one round"), Player->Ammo, 29);
    Player->StartFire(); Player->StopFire();
    TestEqual(TEXT("Rapid repeated RPC cannot bypass fire rate"), Player->Ammo, 29);
    Player->Reload();
    TestTrue(TEXT("Reload begins on server"), Player->bReloading);
    Player->StartFire();
    TestEqual(TEXT("Cannot fire during reload"), Player->Ammo, 29);
    // New timers become active on the following timer-manager tick.
    World->GetTimerManager().Tick(0.0f);
    ++GFrameCounter;
    World->GetTimerManager().Tick(2.0f);
    TestFalse(TEXT("Reload completes"), Player->bReloading);
    TestEqual(TEXT("Reload refills magazine"), Player->Ammo, 30);
    TestEqual(TEXT("Reload conserves total ammunition"), Player->ReserveAmmo, 89);
    FDamageEvent Damage;
    Friend->TakeDamage(25, Damage, nullptr, Player);
    TestEqual(TEXT("Co-op friendly fire is disabled"), Friend->Health, 100.f);
    Enemy->TakeDamage(25, Damage, nullptr, Player);
    TestEqual(TEXT("Player can damage enemy"), Enemy->Health, 75.f);
    Enemy->TakeDamage(100, Damage, nullptr, Player);
    TestTrue(TEXT("Lethal damage kills enemy"), Enemy->bDead);
    Enemy->Reload();
    TestFalse(TEXT("Dead character cannot reload"), Enemy->bReloading);
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif
