#include "TeamGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Ballsack.h"

ATeamGameState::ATeamGameState()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ATeamGameState::BeginPlay()
{
    Super::BeginPlay();
}

static ABallsack* FindBall(UWorld* W, TSubclassOf<AActor> BallClass)
{
    if (!W) return nullptr;

    // If BallClass isn't set in BP, try to auto-load your BP_Ball
    if (!*BallClass)
    {
        BallClass = StaticLoadClass(AActor::StaticClass(), nullptr, TEXT("/Game/BP_Ball.BP_Ball_C"));
    }

    AActor* Found = UGameplayStatics::GetActorOfClass(W, BallClass);
    return Cast<ABallsack>(Found);
}

void ATeamGameState::ResetBallToCentre()
{
    UWorld* W = GetWorld();
    if (ABallsack* Ball = FindBall(W, BallClass))
    {
        const FVector Centre(0.f, 0.f, 105.f);
        Ball->SetActorLocation(Centre, false, nullptr, ETeleportType::TeleportPhysics);
        PossessingTeamID = -1;
    }
}

void ATeamGameState::HandleGoal(int32 ScoringTeamID, bool bRightGoal)
{
    // You can extend this with score tracking per team if you already have it.
    ResetBallToCentre();
    PossessingTeamID = -1;
}

FVector ATeamGameState::GetBallLocationSafe(const UObject* WorldContext)
{
    UWorld* W = WorldContext ? WorldContext->GetWorld() : nullptr;
    if (ABallsack* Ball = FindBall(W, TSubclassOf<AActor>()))
    {
        return Ball->GetActorLocation();
    }
    return FVector::ZeroVector;
}
