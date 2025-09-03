#include "TeamGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Ballsack.h"
#include "FootballTeam.h"

ATeamGameState::ATeamGameState()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATeamGameState::BeginPlay()
{
	Super::BeginPlay();

	Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
	RefreshTeamsIfEmpty();
}

void ATeamGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Ball)
	{
		Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass()));
	}

	const int32 NewPossessing = Ball ? Ball->GetPossessingTeamID() : -1;
	if (NewPossessing != PossessingTeamID)
	{
		PossessingTeamID = NewPossessing;

		for (AFootballTeam* T : Teams)
		{
			if (!T) continue;
			if (PossessingTeamID >= 0 && T->TeamID == PossessingTeamID)
			{
				T->SetTeamState(ETeamState::Attack);
			}
			else
			{
				T->SetTeamState(ETeamState::Defence);
			}
		}
	}
}

void ATeamGameState::RefreshTeamsIfEmpty()
{
	if (Teams.Num() == 0)
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFootballTeam::StaticClass(), Found);
		for (AActor* A : Found)
		{
			if (AFootballTeam* T = Cast<AFootballTeam>(A))
			{
				Teams.Add(T);
			}
		}
	}
}
