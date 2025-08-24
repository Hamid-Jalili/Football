// DefaultGameMode.cpp
#include "DefaultGameMode.h"
#include "OSF.h"
#include "TeamGameState.h"
#include "FootballTeam.h"
#include "FootballerController.h"
#include "Ballsack.h"
#include "Kismet/GameplayStatics.h"

ADefaultGameMode::ADefaultGameMode()
{
	// Nothing special – GameMode classes (BP_GameMode) set in project/world settings
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ADefaultGameMode::StartPlay()
{
	Super::StartPlay();
	StartMatch(); // keep previous lifecycle
}

void ADefaultGameMode::StartMatch()
{
	if (!HasAuthority()) { Super::StartMatch(); return; }

	ATeamGameState* State = GetWorld()->GetGameState<ATeamGameState>();
	check(State);

	// Create teams if missing
	if (!State->HomeTeam) State->HomeTeam = GetWorld()->SpawnActor<AFootballTeam>();
	if (!State->AwayTeam) State->AwayTeam = GetWorld()->SpawnActor<AFootballTeam>();

	// Ensure Ball reference
	State->Ball = ABallsack::GetWorldBall(GetWorld());

	// Spawn players (Blueprint class now)
	State->LoadSampleState();

	// Possess first Home player with local controller (yellow starts)
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		AFootballerController* FC = Cast<AFootballerController>(PC);
		if (FC && State->HomeTeam && State->HomeTeam->Footballers.Num() > 0)
		{
			FC->SwitchToFootballer(State->HomeTeam->Footballers[0]);
		}
	}

	Super::StartMatch();
}
