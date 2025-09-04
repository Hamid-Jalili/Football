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

	/** 0 = Left/Blue, 1 = Right/Red; -1 = none/unknown */
	UFUNCTION(BlueprintCallable, Category = "Teams")
	int32 GetPossessingTeamID() const { return PossessingTeamID; }

	UFUNCTION(BlueprintCallable, Category = "Teams")
	void SetPossessingTeamID(int32 NewID) { PossessingTeamID = NewID; }

	/** All teams present in the match (index with TeamID when possible). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teams")
	TArray<class AFootballTeam*> Teams;

	/** Convenience: returns nullptr if TeamID out of range. */
	UFUNCTION(BlueprintCallable, Category = "Teams")
	class AFootballTeam* GetTeam(int32 TeamID) const;

	/**
	 * Called by BP_Goal on net overlap.
	 * @param ScoringTeamID  Team that scored (0/1)
	 * @param bRightGoal     Which goal volume fired (for debugging/telemetry only)
	 */
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void HandleGoal(int32 ScoringTeamID, bool bRightGoal);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	int32 PossessingTeamID = -1;

	void ResetBallToCentre();
};
