#include "FootballerAIController.h"
#include "Footballer.h"
#include "FootballTeam.h"
#include "Ballsack.h"
#include "TeamGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

AFootballerAIController::AFootballerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFootballerAIController::BeginPlay()
{
	Super::BeginPlay();

	ControlledFootballer = Cast<AFootballer>(GetPawn());
	Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
	TeamGameState = Cast<ATeamGameState>(UGameplayStatics::GetActorOfClass(GetWorld(), ATeamGameState::StaticClass()));

	if (ControlledFootballer)
	{
		if (auto* Move = ControlledFootballer->GetCharacterMovement())
		{
			Move->bUseControllerDesiredRotation = true;
			Move->bOrientRotationToMovement = true;
		}
	}
}

void AFootballerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ControlledFootballer || !Ball) return;

	const FVector Target = ComputeTargetLocation();

	// Smaller acceptance so they keep adjusting; do NOT stop on overlap.
	MoveToLocation(Target, 5.f, true, true, false, true, 0, true);
}

FVector AFootballerAIController::ComputeTargetLocation() const
{
	if (!ControlledFootballer || !ControlledFootballer->TeamRef || !Ball) return FVector::ZeroVector;

	const int32 MyTeamID = ControlledFootballer->GetTeamID();
	const bool bMyTeamInPossession = (TeamGameState && TeamGameState->GetPossessingTeamID() == MyTeamID);

	const int32 Index = ControlledFootballer->PlayerIndex;
	const FVector AnchorAttack = ControlledFootballer->TeamRef->GetAttackAnchor(Index);
	const FVector AnchorDefence = ControlledFootballer->TeamRef->GetDefenceAnchor(Index);
	const FVector Anchor = bMyTeamInPossession ? AnchorAttack : AnchorDefence;

	const FVector BallLocation = Ball->GetActorLocation();

	// DEFENCE — nearest presses, others compact and keep sliding
	if (!bMyTeamInPossession)
	{
		if (IsNearestToBallOnMyTeam())
		{
			return BallLocation;
		}

		const FVector ToBall = (BallLocation - Anchor).GetClampedToMaxSize(900.f);
		const FVector Jitter = ComputeJitter(Index);     // slightly stronger jitter now
		return Anchor + ToBall * 0.35f + Jitter;
	}

	// ATTACK — closest support; others keep shape but lean ball-side
	float Bias = 0.25f;
	if (Index == 9 || Index == 10) Bias = 0.35f; // strikers

	bool bSupportRunner = false;
	{
		float MyDist = FVector::Dist(ControlledFootballer->GetActorLocation(), BallLocation);
		int CloserCount = 0;
		for (AFootballer* Mate : ControlledFootballer->TeamRef->Players)
		{
			if (!Mate || Mate == ControlledFootballer) continue;
			const float D = FVector::Dist(Mate->GetActorLocation(), BallLocation);
			if (D < MyDist) ++CloserCount;
		}
		bSupportRunner = (CloserCount <= 1);
	}
	if (bSupportRunner) Bias = 0.45f;

	const FVector AttackToBall = (BallLocation - Anchor).GetClampedToMaxSize(1200.f);
	const FVector AttackJitter = ComputeJitter(Index);
	return Anchor + AttackToBall * Bias + AttackJitter;
}

bool AFootballerAIController::IsNearestToBallOnMyTeam() const
{
	if (!ControlledFootballer || !ControlledFootballer->TeamRef || !Ball) return false;

	const FVector BallLocation = Ball->GetActorLocation();
	const float MyDist = FVector::Dist(ControlledFootballer->GetActorLocation(), BallLocation);

	for (AFootballer* Mate : ControlledFootballer->TeamRef->Players)
	{
		if (!Mate || Mate == ControlledFootballer) continue;
		const float OtherDist = FVector::Dist(Mate->GetActorLocation(), BallLocation);
		if (OtherDist + 100.f < MyDist)
		{
			return false;
		}
	}
	return true;
}

FVector AFootballerAIController::ComputeJitter(int32 Seed) const
{
	// Increase magnitude to keep them visibly alive
	const float T = GetWorld()->GetTimeSeconds();
	const float S = 0.5f + 0.37f * Seed;
	const float X = 180.f * FMath::Sin(T * (0.6f + 0.11f * S) + 0.7f * Seed);
	const float Y = 180.f * FMath::Cos(T * (0.5f + 0.13f * S) + 0.3f * Seed);
	return FVector(X, Y, 0.f);
}
