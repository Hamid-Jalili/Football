#include "TeamGameState.h"
#include "FootballTeam.h"
#include "Ballsack.h"
#include "Kismet/GameplayStatics.h"

ATeamGameState::ATeamGameState()
{
	PrimaryActorTick.bCanEverTick = false; // AI controllers drive movement
}

void ATeamGameState::BeginPlay()
{
	Super::BeginPlay();
}

AFootballTeam* ATeamGameState::GetTeam(int32 TeamID) const
{
	return Teams.IsValidIndex(TeamID) ? Teams[TeamID] : nullptr;
}

void ATeamGameState::ResetBallToCentre()
{
	if (!GetWorld()) return;

	if (ABallsack* Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass())))
	{
		const FVector Start = Ball->GetActorLocation();
		// Keep current Z to avoid popping; your ABallsack::ResetToCenter should zero velocity inside.
		const FVector Centre(Start.X - Start.X, Start.Y - Start.Y, Start.Z);
		// If your ABallsack has ResetToCenter(FVector), call it; otherwise SetActorLocation + zero physics works too.
		if (Ball->GetClass()->FindFunctionByName(TEXT("ResetToCenter")))
		{
			Ball->ResetToCenter(FVector(0.f, 0.f, Start.Z));
		}
		else
		{
			Ball->SetActorLocation(FVector(0.f, 0.f, Start.Z), false);
			if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ball->GetRootComponent()))
			{
				Prim->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Prim->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
			}
		}
	}
}

void ATeamGameState::HandleGoal(int32 ScoringTeamID, bool bRightGoal)
{
	// 1) Reset ball to centre, stop its motion
	ResetBallToCentre();

	// 2) Possession for kickoff goes to the conceding team
	const int32 ConcedingTeam = (ScoringTeamID == 0) ? 1 : 0;
	SetPossessingTeamID(ConcedingTeam);

	// 3) Do not teleport players — AI controllers will walk/run back to anchors automatically.
	//    (Anchors depend on GetPossessingTeamID(), so the shape flips for kickoff.)
}
