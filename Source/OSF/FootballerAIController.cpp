#include "FootballerAIController.h"

AFootballerAIController::AFootballerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(true);
}

void AFootballerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	MyP = Cast<AFootballer>(InPawn);
}

void AFootballerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!MyP) return;

	const FVector Home = MyP->GetHomeSpot();
	const float Radius = MyP->IsGoalkeeper() ? KeeperAcceptRadius : AcceptRadius;
	DriveToward(Home, DeltaSeconds, Radius);
}

void AFootballerAIController::DriveToward(const FVector& TargetWorld, float /*DeltaSeconds*/, float Accept)
{
	if (!MyP) return;

	const FVector Here = MyP->GetActorLocation();
	FVector To = TargetWorld - Here; To.Z = 0.f;

	if (To.SizeSquared() > FMath::Square(Accept))
	{
		MyP->SetDesiredMovement(To.GetSafeNormal());
		MyP->SetDesiredSprintStrength(MoveSprint);
	}
	else
	{
		MyP->SetDesiredMovement(FVector::ZeroVector);
		MyP->SetDesiredSprintStrength(0.f);
	}
}
