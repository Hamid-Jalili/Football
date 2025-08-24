// TeamGameState.h - FULL FILE
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

class AFootballTeam;
class AFootballer;
class AFootballerAIController;
class ABallsack;

/**
 * Holds match-wide data and spawns teams/players/ball for a simple kickoff.
 */
UCLASS()
class OSF_API ATeamGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ATeamGameState();

	// === Editable Defaults (show up in BP_TeamGameState) ===
	/** 11 by default */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	int32 PlayersPerSide = 11;

	/** Start with the HOME (Yellow) team in control */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	bool bHomeStarts = true;

	/** If true, HOME attacks to +X (right). If false, HOME attacks to -X (left). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Match")
	bool bHomeAttacksRight = true;

	/** Footballer pawn to spawn. Must be your BP_Footballer (or C++ class). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<AFootballer> FootballerClass;

	/** AI controller the non-human players should use. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<AFootballerAIController> FootballerAIControllerClass;

	/** Ball actor to find/spawn. If left null we will search the world for one. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Classes")
	TSubclassOf<ABallsack> BallClass;

	// === Runtime references ===
	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	AFootballTeam* HomeTeam = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	AFootballTeam* AwayTeam = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Runtime")
	ABallsack* Ball = nullptr;

public:
	/** Build a simple scene: teams, players and put the ball on the center spot. */
	UFUNCTION(BlueprintCallable, Category = "Match Setup")
	void LoadSampleState();

protected:
	/** Helper: create team and players, align them for a basic kickoff shape. */
	void BuildTeam(bool bIsHome, int32 NumPlayers, float HalfWidth, float HalfLength);

	/** Compute a kickoff layout for player Index (0-based), returns world location. */
	FVector KickoffLocation(bool bIsHomeSide, int32 Index, float HalfWidth, float HalfLength) const;
};
