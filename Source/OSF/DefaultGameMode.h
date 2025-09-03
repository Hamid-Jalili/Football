#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DefaultGameMode.generated.h"

/**
 * Spawns two teams (11v11 by default), assigns team refs/indexes,
 * and ensures the local player is possessed (Team A, index 0).
 */
UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

	virtual void BeginPlay() override;

protected:
	/** Character class to spawn for each footballer (usually your BP_Footballer) */
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<class AFootballer> FootballerClass;

	/** Team actor class (C++ AFootballTeam or a BP derived from it) */
	UPROPERTY(EditDefaultsOnly, Category = "Setup")
	TSubclassOf<class AFootballTeam> TeamClass;

	/** Players per team (11 for 4-4-2) */
	UPROPERTY(EditAnywhere, Category = "Setup")
	int32 PlayersPerTeam;

	/** World X offset between halves; Team B is mirrored on +X side */
	UPROPERTY(EditAnywhere, Category = "Setup")
	float PitchHalfLengthOffset;
};
