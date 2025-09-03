#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FootballerAIController.generated.h"

UCLASS()
class OSF_API AFootballerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFootballerAIController();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	class AFootballer* ControlledFootballer = nullptr;
	class ABallsack* Ball = nullptr;
	class ATeamGameState* TeamGameState = nullptr;

	FVector ComputeTargetLocation() const;
	bool IsNearestToBallOnMyTeam() const;
	FVector ComputeJitter(int32 Seed) const;
};
