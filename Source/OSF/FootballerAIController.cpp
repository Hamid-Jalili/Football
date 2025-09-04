#include "FootballerAIController.h"
#include "Footballer.h"
#include "FootballTeam.h"
#include "TeamGameState.h"
#include "Ballsack.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"

AFootballerAIController::AFootballerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
}

void AFootballerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Me = Cast<AFootballer>(InPawn);
	if (Me && Me->GetCharacterMovement())
	{
		Me->GetCharacterMovement()->bUseControllerDesiredRotation = true;
		Me->GetCharacterMovement()->bOrientRotationToMovement = true;
	}
}

void AFootballerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Me || !Me->TeamRef) return;
	if (Cast<APlayerController>(Me->GetController())) return; // human-controlled

	RepathCooldown -= DeltaSeconds;
	if (RepathCooldown > 0.f) return;
	RepathCooldown = 0.25f;

	// Possession and ball
	const ATeamGameState* TGS = GetWorld() ? GetWorld()->GetGameState<ATeamGameState>() : nullptr;
	const int32 Possessing = TGS ? TGS->GetPossessingTeamID() : -1;
	const bool bAttacking = (Possessing == Me->TeamRef->TeamID);

	ABallsack* Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
	const FVector BallLoc = Ball ? Ball->GetActorLocation() : FVector::ZeroVector;

	// Tactical target
	const FVector Target = Me->TeamRef->GetAnchor(bAttacking, Me->PlayerIndex, BallLoc);

	// Move if not close
	const float Dist = FVector::Dist2D(Me->GetActorLocation(), Target);
	if (Dist > 120.f)
	{
		MoveToLocation(Target, /*Acceptance*/90.f, true, true, true, false, 0, true);
	}
	else
	{
		StopMovement();
	}
}
