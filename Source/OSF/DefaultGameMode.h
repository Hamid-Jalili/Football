#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Footballer.h"          // ✅ Needed here
#include "FootballTeam.h"
#include "DefaultGameMode.generated.h"

UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pitch")
	float PitchHalfLength = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teams")
	int32 PlayersPerTeam = 11;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teams")
	TSubclassOf<AFootballer> FootballerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Teams")
	int32 HumanTeamID = 0;

private:
	AFootballTeam* EnsureTeamActor(int32 TeamID);
	FVector FormationHome(int32 TeamID, int32 Index) const;
	EFootballRole RoleForIndex(int32 Index);
	void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutList);
};
