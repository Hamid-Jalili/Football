#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FootballerController.generated.h"

class AFootballer;

UCLASS()
class OSF_API AFootballerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFootballerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// ---------------- Settings ----------------
	UPROPERTY(EditAnywhere, Category = "Football")
	int32 ControlledTeamID = 0;

	UPROPERTY(EditAnywhere, Category = "Football")
	float SwitchCooldown = 0.15f;

	UPROPERTY(EditAnywhere, Category = "Football")
	float InitialCacheDelay = 0.25f;

	/** Turn off to silence on-screen messages */
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDebugInput = true;

	// ---------------- Runtime ----------------
	UPROPERTY() TArray<AFootballer*> TeamMates;
	UPROPERTY() AFootballer* CurrentControlled = nullptr;
	UPROPERTY() AActor* BallActor = nullptr;

	float  AxisForward = 0.f;
	float  AxisRight = 0.f;
	double LastSwitchTime = -1000.0;
	double LastTickHudTime = 0.0;

	// Save/restore orientation for pawns we leave
	struct FOrientSnapshot
	{
		bool     bUseControllerRotationYaw = false;
		bool     bOrientRotationToMovement = false;
		FRotator RotationRate = FRotator::ZeroRotator;
	};
	TMap<AFootballer*, FOrientSnapshot> SavedOrient;

	// ---------------- Input handlers ----------------
	void MoveForward(float Value);
	void MoveRight(float Value);
	void SprintPressed();
	void SprintReleased();
	void ShootPressed();
	void PassPressed();

	void SwitchToClosest();
	void CycleNext();
	void CyclePrev();

	// ---------------- Helpers ----------------
	AFootballer* GetControlledFootballer() const;
	void ApplyDesiredMovement();
	void CacheRefs();
	void PossessFootballer(AFootballer* P);
	void ApplyPlayerFacingIfCharacter(AFootballer* P);
	AFootballer* FindClosestToBall() const;
	AFootballer* FindCycle(bool bNext) const;
	void GetCameraBasis(FVector& OutForward, FVector& OutRight) const;
	AActor* FindBallActor() const;

	// Debug
	void Screen(const FString& Msg, const FColor& C = FColor::Green, float T = 1.5f) const;
	void DebugKeyD();
};
