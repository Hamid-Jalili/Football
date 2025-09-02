#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Footballer.h"                     // UHT needs the real type here
#include "FootballerController.generated.h"

UCLASS()
class OSF_API AFootballerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFootballerController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY()
	AFootballer* ControlledFootballer = nullptr;

	// axis state
	float AxisForward = 0.f;
	float AxisRight = 0.f;

	UPROPERTY(EditAnywhere, Category = "Input")
	bool bUseCameraYaw = true;

	// bindings
	void MoveForward(float Val);
	void MoveRight(float Val);
	void SprintPressed();
	void SprintReleased();
	void Shoot();
	void Pass();
	void KnockOn();

	FVector BuildDesiredMove() const;
};
