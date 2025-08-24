// DefaultGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"   // If you derive from AGameModeBase in your project, switch include/base accordingly.
#include "DefaultGameMode.generated.h"

class ABallsack;
class AFootballTeam;
class AFootballerController;
class ATeamGameState;

/**
 * Owns match flow. Hands first local controller to Home (Yellow),
 * calls TeamGameState to spawn both teams, ball on center, etc.
 */
UCLASS()
class OSF_API ADefaultGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

	virtual void BeginPlay() override;
	virtual void StartPlay() override;
	virtual void StartMatch() override;

	/** Alternate subsequent controllers Home/Away when multiple locals exist */
	bool NextControllerIsHome = true;

	/** Set once we’ve spawned teams to avoid placing controllers too early */
	bool bIsSetup = false;

private:
	void PlaceController(AFootballerController* Controller);
};
