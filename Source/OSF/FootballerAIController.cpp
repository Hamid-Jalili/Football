#include "FootballerAIController.h"
#include "Footballer.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "GameFramework/Actor.h"

AFootballerAIController::AFootballerAIController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFootballerAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Me = Cast<AFootballer>(InPawn);
}

void AFootballerAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Keep AI passive for now to avoid gameplay regressions.
	// (Add your movement/decision logic here when ready.)
	if (!Me.IsValid())
	{
		return;
	}
}
