#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DefaultGameMode.generated.h"

class AFootballTeam;
class AFootballer;

UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADefaultGameMode();

protected:
    virtual void BeginPlay() override;

private:
    void SetInitialViewTarget();
    void SpawnTeams();
    void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers);

public:
    UPROPERTY(EditAnywhere, Category = "Teams")
    TSubclassOf<AFootballTeam> TeamClass;

    UPROPERTY(EditAnywhere, Category = "Teams")
    TSubclassOf<AFootballer> PlayerClass;

    UPROPERTY(EditAnywhere, Category = "Teams")
    int32 NumPlayersPerTeam = 11;

    UPROPERTY(EditAnywhere, Category = "Teams")
    UMaterialInterface* Team0Material;

    UPROPERTY(EditAnywhere, Category = "Teams")
    UMaterialInterface* Team1Material;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDrawFormationDebug = false;
};
