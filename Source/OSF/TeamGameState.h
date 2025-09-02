#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

UCLASS()
class OSF_API ATeamGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATeamGameState();

	/** Optional: assign goal actors in the level (by reference or tag fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	AActor* HomeGoalActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	AActor* AwayGoalActor = nullptr;

	/** Optional: fixed goal locations if you don't place actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	FVector HomeGoalLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	FVector AwayGoalLocation = FVector::ZeroVector;

	/** Rectangle for the goalkeeper to patrol (depth and half width). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	float KeeperBoxDepth = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Field")
	float KeeperBoxHalfWidth = 900.f;

	UFUNCTION(BlueprintCallable, Category = "Field")
	FVector GetHomeGoalLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Field")
	FVector GetAwayGoalLocation() const;
};
