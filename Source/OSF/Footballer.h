#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

class ABallsack;
class AFootballTeam;

#include "Footballer.generated.h" // must be the last include

UENUM(BlueprintType)
enum class EFootballRole : uint8
{
	GK  UMETA(DisplayName = "Goalkeeper"),
	DEF UMETA(DisplayName = "Defender"),
	MID UMETA(DisplayName = "Midfielder"),
	FWD UMETA(DisplayName = "Forward")
};

UCLASS()
class OSF_API AFootballer : public ACharacter
{
	GENERATED_BODY()

public:
	AFootballer();

	// AActor / ACharacter
	virtual void BeginPlay() override;

	/** 0 = Left/Blue, 1 = Right/Red */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	EFootballRole PlayerRole = EFootballRole::MID;

	/** Where this player should return to (spawn/formation). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	FVector HomeLocation = FVector::ZeroVector;

	// --- Desired control (read/write via functions) ---
	UFUNCTION(BlueprintCallable, Category = "Control")
	void SetDesiredLocation(const FVector& In);

	UFUNCTION(BlueprintCallable, Category = "Control")
	void SetDesiredMovement(const FVector& In);

	UFUNCTION(BlueprintCallable, Category = "Control")
	void SetDesiredSprintStrength(float In);

	UFUNCTION(BlueprintPure, Category = "Control")
	FVector GetDesiredMove() const { return DesiredMove; }

	// --- Ball helpers ---
	UFUNCTION(BlueprintCallable, Category = "Ball")
	ABallsack* FindBall() const;

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ShootBall(float Power, const FVector& Dir);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void PassBall(float Power, const FVector& Dir);

	// --- Steering helper used by AI ---
	UFUNCTION(BlueprintCallable, Category = "AI")
	FVector GetSeparationCorrection(const TArray<AFootballer*>& Neighbors, float Radius) const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Control")
	FVector DesiredLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Control")
	FVector DesiredMove = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Control")
	float DesiredSprint = 0.f;
};
