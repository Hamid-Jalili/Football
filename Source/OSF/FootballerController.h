// FootballerController.h

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FootballerController.generated.h"

class AFootballer;

/**
 * Player controller that translates legacy Input (WASD/Pad) into
 * camera-relative movement and pushes it to the possessed AFootballer.
 */
UCLASS()
class OSF_API AFootballerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFootballerController();

	// APlayerController
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// Current controlled pawn (read-only outside)
	UFUNCTION(BlueprintCallable, Category = "Control")
	AFootballer* GetControlledFootballer() const { return ControlledFootballer; }

	// Switch to a specific footballer (called by GameMode or by us)
	UFUNCTION(BlueprintCallable, Category = "Control")
	void SwitchToFootballer(AFootballer* NewFootballer);

protected:
	// ----- Axis input -----
	void AxisMoveForward(float Value);
	void AxisMoveRight(float Value);
	void AxisSprint(float Value);

	// ----- Actions -----
	void ActionShoot();
	void ActionPass();
	void ActionThrough();
	void ActionSwitchPlayer();

	// Build world-space (camera-relative) desired move from axis values
	FVector BuildDesiredMovement() const;

	// Find a reasonable teammate to switch to (very simple cycle)
	AFootballer* FindNextTeammate() const;

protected:
	// Stored axis values
	float ForwardAxis = 0.f;  // W/S (+W, -S)
	float RightAxis = 0.f;  // D/A (+D, -A)
	float SprintAxis = 0.f;  // Space/Gamepad RT (0..1)

	// The footballer we currently possess/control
	UPROPERTY()
	AFootballer* ControlledFootballer = nullptr;
};
