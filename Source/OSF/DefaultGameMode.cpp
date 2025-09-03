#include "DefaultGameMode.h"

#include "FootballTeam.h"
#include "Footballer.h"
#include "FootballerAIController.h"
#include "FootballerController.h"
#include "TeamGameState.h"

#include "Kismet/GameplayStatics.h"

ADefaultGameMode::ADefaultGameMode()
{
	PlayersPerTeam = 11;
	PitchHalfLengthOffset = 8000.f;

	DefaultPawnClass = nullptr;
	PlayerControllerClass = AFootballerController::StaticClass();
	GameStateClass = ATeamGameState::StaticClass();
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (!FootballerClass || !TeamClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("DefaultGameMode: FootballerClass or TeamClass not set."));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AFootballTeam* TeamA = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	AFootballTeam* TeamB = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!TeamA || !TeamB) return;

	TeamA->TeamID = 0;
	TeamB->TeamID = 1;

	// --- Team A (left) ---
	for (int32 i = 0; i < PlayersPerTeam; ++i)
	{
		const FVector SpawnLoc = TeamA->GetAttackAnchor(i);
		AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, FRotator::ZeroRotator, Params);
		if (!P) continue;

		P->TeamRef = TeamA;
		P->PlayerIndex = i;
		TeamA->Players.Add(P);

		if (i > 0)
		{
			if (AAIController* C = GetWorld()->SpawnActor<AFootballerAIController>(AFootballerAIController::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params))
			{
				C->Possess(P);
			}
		}
	}

	// --- Team B (right) - just +PitchHalfLengthOffset on X, rotate 180 ---
	for (int32 i = 0; i < PlayersPerTeam; ++i)
	{
		const FVector SpawnLoc = TeamB->GetAttackAnchor(i) + FVector(PitchHalfLengthOffset, 0.f, 0.f);
		AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, FRotator(0.f, 180.f, 0.f), Params);
		if (!P) continue;

		P->TeamRef = TeamB;
		P->PlayerIndex = i;
		TeamB->Players.Add(P);

		if (AAIController* C = GetWorld()->SpawnActor<AFootballerAIController>(AFootballerAIController::StaticClass(), SpawnLoc, FRotator::ZeroRotator, Params))
		{
			C->Possess(P);
		}
	}

	// Register teams with GameState
	if (ATeamGameState* TGS = Cast<ATeamGameState>(UGameplayStatics::GetActorOfClass(GetWorld(), ATeamGameState::StaticClass())))
	{
		TGS->Teams.Empty();
		TGS->Teams.Add(TeamA);
		TGS->Teams.Add(TeamB);
	}

	// Possess Team A, index 0 by default
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if (TeamA && TeamA->Players.Num() > 0 && TeamA->Players[0])
		{
			PC->Possess(TeamA->Players[0]);
		}
	}
}
