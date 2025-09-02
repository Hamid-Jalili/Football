#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Footballer.h"                        // UHT needs the real type here
#include "FootballerAIController.generated.h"

UCLASS()
class OSF_API AFootballerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFootballerAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY()
	AFootballer* MyP = nullptr;

	UPROPERTY(EditAnywhere, Category = "AI")
	float AcceptRadius = 120.f;

	UPROPERTY(EditAnywhere, Category = "AI")
	float MoveSprint = 0.35f;

	UPROPERTY(EditAnywhere, Category = "AI")
	float KeeperAcceptRadius = 60.f;

	void DriveToward(const FVector& TargetWorld, float DeltaSeconds, float Accept);
};
