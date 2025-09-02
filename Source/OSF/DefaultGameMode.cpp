#include "DefaultGameMode.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

#include "FootballerAIController.h"
#include "FootballerController.h"
#include "TeamGameState.h"

ADefaultGameMode::ADefaultGameMode()
{
	DefaultPawnClass = nullptr; // possess a spawned striker in PostLogin
	PlayerControllerClass = AFootballerController::StaticClass();
	GameStateClass = ATeamGameState::StaticClass();
	AIControllerClass = AFootballerAIController::StaticClass();

	// Prefer Blueprint pawn (mesh/anim); fallback to C++.
	ConstructorHelpers::FClassFinder<AFootballer> BP1(TEXT("/Game/BP_Footballer"));
	if (BP1.Succeeded())
	{
		FootballerClass = BP1.Class;
	}
	else
	{
		ConstructorHelpers::FClassFinder<AFootballer> BP2(TEXT("/Game/Blueprints/BP_Footballer"));
		FootballerClass = BP2.Succeeded() ? BP2.Class : AFootballer::StaticClass();
	}
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	const float HalfX = PitchHalfLength * 0.35f;
	const FVector HomeBase(-HalfX, 0.f, 100.f);
	const FVector AwayBase(+HalfX, 0.f, 100.f);

	SpawnTeam(0, HomeBase, true);
	SpawnTeam(1, AwayBase, false);
}

void ADefaultGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	if (!NewPlayer) return;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFootballer::StaticClass(), Found);

	for (AActor* A : Found)
	{
		AFootballer* F = Cast<AFootballer>(A);
		if (!F) continue;

		// Possess a home outfield player (not GK)
		if (F->GetTeamIndex() == 0 && !F->IsGoalkeeper())
		{
			if (APawn* P = Cast<APawn>(F))
			{
				if (AController* C = P->GetController())
				{
					C->UnPossess();
				}
				NewPlayer->Possess(P);
				break;
			}
		}
	}
}

void ADefaultGameMode::SpawnTeam(int32 TeamIndex, const FVector& HalfBase, bool bHomeTeam)
{
	if (!FootballerClass || !AIControllerClass) return;
	UWorld* World = GetWorld();
	if (!World) return;

	const FRotator FacingRot = bHomeTeam ? FRotator(0.f, 0.f, 0.f) : FRotator(0.f, 180.f, 0.f);

	const int32 NumPlayers = 11;
	for (int32 i = 0; i < NumPlayers; ++i)
	{
		const FVector Offset = FormationOffset(i, bHomeTeam);
		const FVector SpawnLoc = HalfBase + Offset;

		const float ClampedY = FMath::Clamp(SpawnLoc.Y, -PitchHalfWidth * 0.9f, PitchHalfWidth * 0.9f);
		const FVector FinalLoc(SpawnLoc.X, ClampedY, SpawnLoc.Z);

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AFootballer* NewF = World->SpawnActor<AFootballer>(FootballerClass, FinalLoc, FacingRot, Params);
		if (!NewF) continue;

		NewF->SetTeamIndex(TeamIndex);
		NewF->SetHomeSpot(FinalLoc);              // AI formation anchor
		if (i == 0) { NewF->SetIsGoalkeeper(true); }

		AFootballerAIController* AIC = World->SpawnActor<AFootballerAIController>(AIControllerClass, FinalLoc, FacingRot, Params);
		if (AIC)
		{
			AIC->Possess(NewF);
		}
	}
}

FVector ADefaultGameMode::FormationOffset(int32 RoleIndex, bool bHomeTeam) const
{
	const float dir = bHomeTeam ? +1.f : -1.f;

	auto Make = [&](float DepthSteps, float LateralSteps)
		{
			const float X = DepthSteps * LineDepth * dir;   // toward center line
			const float Y = LateralSteps * Lateral;
			return FVector(X, Y, 0.f);
		};

	if (RoleIndex == 0) return Make(0.f, 0.f);      // GK

	if (RoleIndex >= 1 && RoleIndex <= 4)           // DEF 4
	{
		const int32 idx = RoleIndex - 1;
		return Make(2.f, idx - 1.5f);
	}

	if (RoleIndex >= 5 && RoleIndex <= 8)           // MID 4
	{
		const int32 idx = RoleIndex - 5;
		return Make(4.f, idx - 1.5f);
	}

	// FWD 2
	const int32 idx = RoleIndex - 9;
	return Make(6.f, (idx - 0.5f) * 2.f);
}
