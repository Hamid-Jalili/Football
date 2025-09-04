// FootballTeam.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballTeam.generated.h"

class AFootballer;

UCLASS()
class OSF_API AFootballTeam : public AActor
{
	GENERATED_BODY()

public:
	AFootballTeam();

	/** 0 = left team, 1 = right team */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = 0;

	/** Team roster (Blueprint-friendly pointers) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	TArray<AFootballer*> Players;

	/** Registers a player at a specific squad index (Blueprint-callable) */
	UFUNCTION(BlueprintCallable, Category = "Team")
	void RegisterPlayer(AFootballer* Player, int32 SquadIndex);

	/** C++ convenience: append and forward to the indexed version (NOT a UFUNCTION) */
	void RegisterPlayer(AFootballer* Player);

	/** Re-assign roles to any players that still have the default role */
	UFUNCTION(BlueprintCallable, Category = "Team")
	void AssignRoles();

protected:
	virtual void BeginPlay() override;
};
