#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FootballerController.generated.h"

UCLASS()
class OSF_API AFootballerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFootballerController();

	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	class AFootballer* ControlledFootballer = nullptr;

	FVector2D MoveInput = FVector2D::ZeroVector;
	bool bSprint = false;

	void MoveForward(float Value);
	void MoveRight(float Value);
	void SprintPressed();
	void SprintReleased();
	void ShootPressed();
	void PassPressed();

	/** Switch to nearest teammate to the ball (excluding current). */
	void SwitchToNearestTeammate();

	/** WASD relative to camera yaw. */
	FVector BuildDesiredMove() const;
};
