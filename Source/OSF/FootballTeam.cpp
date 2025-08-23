#include "FootballTeam.h"
#include "Ballsack.h"
#include "Kismet/GameplayStatics.h"

AFootballTeam::AFootballTeam()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFootballTeam::BeginPlay()
{
	Super::BeginPlay();
}

void AFootballTeam::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

AFootballer* AFootballTeam::GetClosestFootballerToBall(ABallsack* Ball)
{
	if (!Ball) return nullptr;

	AFootballer* Closest = nullptr;
	float BestDistSq = FLT_MAX;

	for (AFootballer* F : Footballers)
	{
		if (!F) continue;
		const float DistSq = FVector::DistSquared(F->GetActorLocation(), Ball->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Closest = F;
		}
	}

	return Closest;
}
