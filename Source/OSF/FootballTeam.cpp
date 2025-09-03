#include "FootballTeam.h"
#include "Footballer.h"

AFootballTeam::AFootballTeam()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFootballTeam::BeginPlay()
{
	Super::BeginPlay();
	InitializeFormations();
}

void AFootballTeam::SetTeamState(ETeamState NewState)
{
	TeamState = NewState;
}

void AFootballTeam::InitializeFormations()
{
	// Index map: 0 GK, 1 LB, 2 LCB, 3 RCB, 4 RB, 5 LM, 6 LCM, 7 RCM, 8 RM, 9 LS, 10 RS

	AttackFormation = {
		FVector(-4000,   0, 0),   // GK
		FVector(-3000, -1500, 0), // LB
		FVector(-3000,  -500, 0), // LCB
		FVector(-3000,   500, 0), // RCB
		FVector(-3000,  1500, 0), // RB
		FVector(-1500, -1200, 0), // LM
		FVector(-1500,  -400, 0), // LCM
		FVector(-1500,   400, 0), // RCM
		FVector(-1500,  1200, 0), // RM
		FVector(0,  -600, 0),// LS
		FVector(0,   600, 0) // RS
	};

	DefenceFormation = {
		FVector(-4500,   0, 0),   // GK
		FVector(-3800, -1600, 0), // LB
		FVector(-3800,  -500, 0), // LCB
		FVector(-3800,   500, 0), // RCB
		FVector(-3800,  1600, 0), // RB
		FVector(-2200, -1300, 0), // LM
		FVector(-2200,  -400, 0), // LCM
		FVector(-2200,   400, 0), // RCM
		FVector(-2200,  1300, 0), // RM
		FVector(-800,  -700, 0), // LS
		FVector(-800,   700, 0)  // RS
	};
}

FVector AFootballTeam::GetAnchorLocation(int32 PlayerIndex) const
{
	return (TeamState == ETeamState::Attack) ? GetAttackAnchor(PlayerIndex) : GetDefenceAnchor(PlayerIndex);
}

FVector AFootballTeam::GetAttackAnchor(int32 PlayerIndex) const
{
	return AttackFormation.IsValidIndex(PlayerIndex) ? AttackFormation[PlayerIndex] : FVector::ZeroVector;
}

FVector AFootballTeam::GetDefenceAnchor(int32 PlayerIndex) const
{
	return DefenceFormation.IsValidIndex(PlayerIndex) ? DefenceFormation[PlayerIndex] : FVector::ZeroVector;
}
