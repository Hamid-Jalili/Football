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
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY() class AFootballer* Me = nullptr;
	float RepathCooldown = 0.f;
};
