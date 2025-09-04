#include "FootballTeam.h"
#include "Footballer.h"

AFootballTeam::AFootballTeam()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AFootballTeam::RegisterPlayer(AFootballer* P, int32 SquadIdx)
{
	if (!P) return;

	// Keep unique list
	if (!Players.Contains(P))
	{
		Players.Add(P);
	}

	// Make sure team id on player matches this team
	P->TeamID = TeamID;

	// Assign a simple role by index if GameMode didn't assign yet.
	// (Your previous code used MID as the "default/unassigned" sentinel; preserving that behavior.)
	if (P->PlayerRole == EFootballRole::MID)
	{
		if (SquadIdx == 0)                              P->PlayerRole = EFootballRole::GK;
		else if (SquadIdx >= 1 && SquadIdx <= 4)       P->PlayerRole = EFootballRole::DEF;
		else if (SquadIdx >= 5 && SquadIdx <= 8)       P->PlayerRole = EFootballRole::MID;
		else                                           P->PlayerRole = EFootballRole::FWD;
	}
}
