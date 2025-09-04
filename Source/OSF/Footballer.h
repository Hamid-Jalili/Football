#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Footballer.generated.h"

UCLASS()
class OSF_API AFootballer : public ACharacter
{
	GENERATED_BODY()

public:
	AFootballer();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Index 0..10 in formation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 PlayerIndex = 0;

	/** Team reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	class AFootballTeam* TeamRef = nullptr;

	int32 GetTeamID() const;

	/** Control API from controller */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredMovement(const FVector& DesiredMoveWorld);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredSprintStrength(float InStrength);

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void ShootBall(float Power, const FVector& DirectionWorld);

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void PassBall(float Power, const FVector& DirectionWorld);

	/** Called by GameMode after TeamRef is set to apply correct kit */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void ApplyTeamMaterial();

	/** --- Reset/Kickoff support ---------------------------- */

	/** For X seconds, AI will strongly prefer this location (run back to spot) */
	void SetTempMoveTarget(const FVector& Location, float DurationSeconds);

	/** Does this player currently have a temporary target? */
	bool HasTempMoveTarget() const;

	/** If there is a temp target, returns it and true. */
	bool GetTempMoveTarget(FVector& Out) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	class UMaterialInterface* TeamAMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	class UMaterialInterface* TeamBMaterial = nullptr;

private:
	// movement intent (updated by controller)
	FVector LastDesiredMove = FVector::ZeroVector;
	float   DesiredSprintStrength = 0.f;

	// Close-control (directional dribble)
	UPROPERTY(EditAnywhere, Category = "Control")
	float ControlRadius = 150.f;

	UPROPERTY(EditAnywhere, Category = "Control")
	float ControlOffsetForward = 95.f;

	UPROPERTY(EditAnywhere, Category = "Control")
	float ControlResponsiveness = 24.f;

	UPROPERTY(EditAnywhere, Category = "Control")
	float MaxBallSpeed = 1800.f;

	UPROPERTY(EditAnywhere, Category = "Control")
	float GroundAdhesion = 50.f;

	float CloseControlReenableTime = 0.f;

	// --- Temporary target state (used by resets/kickoff) ---
	FVector TempTargetLocation = FVector::ZeroVector;
	float   TempTargetUntilTime = 0.f;

	class ABallsack* FindBall() const;
	void UpdateCloseControl(float DeltaSeconds);
	void DisableCloseControl(float Duration);
};
