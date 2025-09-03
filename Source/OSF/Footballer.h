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

	/** Index 0..10 in formation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 PlayerIndex = 0;

	/** Team reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	class AFootballTeam* TeamRef = nullptr;

	int32 GetTeamID() const;

	/** --- Player control API expected by FootballerController.cpp --- */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredMovement(const FVector& DesiredMoveWorld);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredSprintStrength(float InStrength);

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void ShootBall(float Power, const FVector& DirectionWorld);

	UFUNCTION(BlueprintCallable, Category = "Actions")
	void PassBall(float Power, const FVector& DirectionWorld);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	class UMaterialInterface* TeamAMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Appearance")
	class UMaterialInterface* TeamBMaterial = nullptr;

private:
	/** Cached sprint factor [0..1] */
	float DesiredSprintStrength = 0.f;

	/** Helper: find the ball actor */
	class ABallsack* FindBall() const;
};
