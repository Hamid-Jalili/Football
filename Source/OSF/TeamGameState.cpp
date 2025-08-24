// TeamGameState.cpp - FULL FILE
#include "TeamGameState.h"
#include "FootballTeam.h"
#include "Footballer.h"
#include "FootballerAIController.h"
#include "Ballsack.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerStart.h"

ATeamGameState::ATeamGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATeamGameState::LoadSampleState()
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Create teams (simple AActors that hold arrays of players)
	if (!HomeTeam) HomeTeam = World->SpawnActor<AFootballTeam>();
	if (!AwayTeam) AwayTeam = World->SpawnActor<AFootballTeam>();

	// Find or spawn the ball
	if (!Ball)
	{
		// Prefer an already placed ball in the level
		for (TActorIterator<ABallsack> It(World); It; ++It)
		{
			Ball = *It;
			break;
		}
		// If still none and a class is specified, spawn one at center
		if (!Ball && BallClass)
		{
			Ball = World->SpawnActor<ABallsack>(BallClass, FVector::ZeroVector, FRotator::ZeroRotator);
		}
	}

	// Field extents (match your BP_Field size). Adjust as needed.
	const float HalfLength = 4500.f; // X half-length (goal-to-center)
	const float HalfWidth = 3000.f; // Y half-width

	// Build sides
	BuildTeam(/*bIsHome=*/true, PlayersPerSide, HalfWidth, HalfLength);
	BuildTeam(/*bIsHome=*/false, PlayersPerSide, HalfWidth, HalfLength);

	// Put ball on center spot
	if (Ball)
	{
		Ball->SetActorLocation(FVector(0.f, 0.f, Ball->GetActorLocation().Z));
		Ball->SetActorHiddenInGame(false);
	}

	// If home starts, optionally we could set a flag on home’s first player, etc.
}

void ATeamGameState::BuildTeam(bool bIsHome, int32 NumPlayers, float HalfWidth, float HalfLength)
{
	UWorld* World = GetWorld();
	if (!World || !FootballerClass) return;

	AFootballTeam*& TeamRef = bIsHome ? HomeTeam : AwayTeam;
	if (!TeamRef) TeamRef = World->SpawnActor<AFootballTeam>();
	if (!TeamRef) return;

	TeamRef->Footballers.Empty();

	for (int32 i = 0; i < NumPlayers; ++i)
	{
		const FVector SpawnLoc = KickoffLocation(bIsHome, i, HalfWidth, HalfLength);
		const FRotator SpawnRot = FRotator(0.f, bIsHome ? (bHomeAttacksRight ? 0.f : 180.f)
			: (bHomeAttacksRight ? 180.f : 0.f), 0.f);

		AFootballer* F = World->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, SpawnRot);
		if (!F) continue;

		F->SetActorHiddenInGame(false);   // ensure visible
		F->SetActorEnableCollision(true);

		// Give them an AI if available
		if (FootballerAIControllerClass)
		{
			AFootballerAIController* AI = World->SpawnActor<AFootballerAIController>(FootballerAIControllerClass, SpawnLoc, SpawnRot);
			if (AI)
			{
				AI->Possess(F);
			}
		}

		// Add to team
		TeamRef->Footballers.Add(F);

		// Mark team pointer if your AFootballer exposes it
		F->Team = TeamRef;
	}
}

FVector ATeamGameState::KickoffLocation(bool bIsHomeSide, int32 Index, float HalfWidth, float HalfLength) const
{
	// Simple 4-4-2-ish lines (GK + 4 defenders + 4 mids + 2 forwards)
	// Index 0: GK, 1-4 DEF, 5-8 MID, 9-10 FWD, rest if any spread in midfield
	const bool bHomeOnRight = bHomeAttacksRight;
	const float SideSign = (bIsHomeSide == bHomeOnRight) ? -1.f : 1.f; // put team on their own half

	float X = SideSign * HalfLength * 0.65f; // base line
	float Y = 0.f;

	if (Index == 0)
	{
		// GK near penalty area
		X = SideSign * HalfLength * 0.9f;
		Y = 0.f;
	}
	else if (Index >= 1 && Index <= 4)
	{
		// 4 defenders line
		X = SideSign * HalfLength * 0.7f;
		const float Slots[4] = { -0.6f, -0.2f, 0.2f, 0.6f };
		Y = Slots[Index - 1] * HalfWidth;
	}
	else if (Index >= 5 && Index <= 8)
	{
		// 4 midfielders
		X = SideSign * HalfLength * 0.45f;
		const float Slots[4] = { -0.7f, -0.25f, 0.25f, 0.7f };
		Y = Slots[Index - 5] * HalfWidth;
	}
	else
	{
		// forwards (or any extra beyond 10)
		X = SideSign * HalfLength * 0.2f;
		if (Index == 9)       Y = -0.25f * HalfWidth;
		else if (Index == 10) Y = 0.25f * HalfWidth;
		else                  Y = 0.f;
	}

	return FVector(X, Y, 0.f);
}
