#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Footballer.h"            // ✅ Keep this (Players is UPROPERTY of AFootballer*)
#include "FootballTeam.generated.h"

UCLASS()
class OSF_API AFootballTeam : public AActor
{
	GENERATED_BODY()

public:
	AFootballTeam();

	/** 0 = Left/Blue, 1 = Right/Red */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = 0;

	/** Runtime list of players belonging to this team */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	TArray<AFootballer*> Players;

	/** Add/register a player with this team, and set per-squad defaults */
	UFUNCTION(BlueprintCallable, Category = "Team")
	void RegisterPlayer(AFootballer* P, int32 SquadIdx);
};
