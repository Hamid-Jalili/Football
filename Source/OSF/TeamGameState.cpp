#include "TeamGameState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ATeamGameState::ATeamGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

FVector ATeamGameState::GetHomeGoalLocation() const
{
	if (HomeGoalActor)
	{
		return HomeGoalActor->GetActorLocation();
	}

	// Fallback: look for an actor with tag "HomeGoal"
	if (const UWorld* W = GetWorld())
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(const_cast<UWorld*>(W), FName(TEXT("HomeGoal")), Found);
		if (Found.Num() > 0)
		{
			return Found[0]->GetActorLocation();
		}
	}

	// Final fallback: configured vector
	return HomeGoalLocation;
}

FVector ATeamGameState::GetAwayGoalLocation() const
{
	if (AwayGoalActor)
	{
		return AwayGoalActor->GetActorLocation();
	}

	// Fallback: look for an actor with tag "AwayGoal"
	if (const UWorld* W = GetWorld())
	{
		TArray<AActor*> Found;
		UGameplayStatics::GetAllActorsWithTag(const_cast<UWorld*>(W), FName(TEXT("AwayGoal")), Found);
		if (Found.Num() > 0)
		{
			return Found[0]->GetActorLocation();
		}
	}

	// Final fallback: configured vector
	return AwayGoalLocation;
}
