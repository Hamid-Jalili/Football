// FootballTeam.cpp
#include "FootballTeam.h"
#include "Footballer.h"

AFootballTeam::AFootballTeam()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFootballTeam::BeginPlay()
{
	Super::BeginPlay();

	// Ensure any pre-placed Players in the array get proper team data at start.
	for (int32 i = 0; i < Players.Num(); ++i)
	{
		if (AFootballer* P = Players[i])
		{
			P->TeamRef = this;
			P->TeamID = TeamID;
			P->ApplyTeamMaterial(TeamID);
		}
	}

	AssignRoles();
}

void AFootballTeam::RegisterPlayer(AFootballer* Player)
{
	if (!Player) return;

	const int32 SquadIdx = Players.AddUnique(Player);
	RegisterPlayer(Player, SquadIdx);
}

void AFootballTeam::RegisterPlayer(AFootballer* Player, int32 SquadIndex)
{
	if (!Player) return;

	// Ensure Players[SquadIndex] exists and matches
	if (!Players.IsValidIndex(SquadIndex))
	{
		const int32 OldNum = Players.Num();
		Players.SetNum(SquadIndex + 1);
		for (int32 i = OldNum; i < Players.Num(); ++i)
		{
			// leave gaps as nullptr if any, we’ll set the slot below
		}
	}
	Players[SquadIndex] = Player;

	// Back-refs & TeamID
	Player->TeamRef = this;
	Player->TeamID = TeamID;

	// If still default role (we treat MID as default), assign by index
	if (Player->PlayerRole == EFootballRole::MID)
	{
		if (SquadIndex == 0)                         Player->PlayerRole = EFootballRole::GK;
		else if (SquadIndex >= 1 && SquadIndex <= 4) Player->PlayerRole = EFootballRole::DEF;
		else if (SquadIndex >= 5 && SquadIndex <= 8) Player->PlayerRole = EFootballRole::MID;
		else                                         Player->PlayerRole = EFootballRole::FWD;
	}

	// Apply the kit for this team
	Player->ApplyTeamMaterial(TeamID);
}

void AFootballTeam::AssignRoles()
{
	for (int32 i = 0; i < Players.Num(); ++i)
	{
		AFootballer* P = Players[i];
		if (!P) continue;

		// Only assign if still the default placeholder role
		if (P->PlayerRole == EFootballRole::MID)
		{
			if (i == 0)                          P->PlayerRole = EFootballRole::GK;
			else if (i >= 1 && i <= 4)          P->PlayerRole = EFootballRole::DEF;
			else if (i >= 5 && i <= 8)          P->PlayerRole = EFootballRole::MID;
			else                                 P->PlayerRole = EFootballRole::FWD;
		}

		// Make sure kit matches TeamID
		P->ApplyTeamMaterial(TeamID);
	}
}
