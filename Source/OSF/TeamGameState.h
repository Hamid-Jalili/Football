#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

class ABallsack;

/**
 * Holds game-wide helpers/state. Minimal but includes the methods referenced by UHT.
 */
UCLASS()
class OSF_API ATeamGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATeamGameState();

	/** Optional override for the ball blueprint/class. If unset, we load /Game/BP_Ball.BP_Ball_C */
	UPROPERTY(EditDefaultsOnly, Category = "Ball")
	TSubclassOf<AActor> BallClass;

	/** Return the ball actor if found (using BallClass or default path) */
	UFUNCTION(BlueprintPure, Category = "Ball")
	ABallsack* GetBall() const;

	/** Move ball to centre spot and stop its motion */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ResetBallToCentre() const;

	/** Basic goal handler – currently just recentres the ball. Extend as needed. */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	void HandleGoal(int32 ScoringTeamID, bool bRightGoal);

	/** Quick world-agnostic way to read ball location for UI/AI */
	UFUNCTION(BlueprintCallable, Category = "Ball")
	static FVector GetBallLocationSafe(const UObject* WorldContext);
};
