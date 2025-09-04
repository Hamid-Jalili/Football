#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DefaultGameMode.generated.h"

UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADefaultGameMode();
	virtual void BeginPlay() override;

	/** Half-pitch length in world units (centre at X=0). Matches BP_Field. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch")
	float PitchHalfLength = 8000.f;

	/** Players per team. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teams")
	int32 PlayersPerTeam = 11;

	/** Class used for auto-spawn. Set this to BP_Footballer in BP_GameMode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teams")
	TSubclassOf<class AFootballer> FootballerClass;

	/** Which team the human controls: 0 = Left/Blue, 1 = Right/Red. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teams")
	int32 HumanTeamID = 0;

private:
	void BuildTeamsFromWorld();
	void EnsureAIPossession(class AFootballer* P);
};
