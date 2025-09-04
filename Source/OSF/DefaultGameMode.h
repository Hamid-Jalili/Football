// DefaultGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DefaultGameMode.generated.h"

class AFootballer;
class AFootballTeam;
class ABallsack;

/**
 * Spawns two teams, assigns roles, spawns the ball, etc.
 */
UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

protected:
	virtual void BeginPlay() override;

	/** Spawn both teams and their players */
	void SpawnTeams();

	/** Spawn one footballer for a given team */
	void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers);

	// ------------------------------
	// Helpers (pure functions)
	// ------------------------------
	/** 11 local formation spots (for left team on -X to +X, centered at origin) */
	void BuildBaseFormation();

	/** Get local spot / world conversions / clamping / facing */
	FVector FormationLocal(int32 Index) const;
	FVector ToWorld(const FVector& Local) const;
	FVector ClampToField(const FVector& P) const;
	float   TeamHalfAngle(int32 TeamID) const;

protected:
	// ----------------------------------------------------
	// Editable classes (set in BP_GameMode)
	// ----------------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AFootballer> FootballerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AFootballTeam> TeamClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ABallsack> BallClass;

	// ----------------------------------------------------
	// Runtime state
	// ----------------------------------------------------
	UPROPERTY() AFootballTeam* Team0 = nullptr;
	UPROPERTY() AFootballTeam* Team1 = nullptr;

	UPROPERTY() TArray<AFootballer*> Team0Players;
	UPROPERTY() TArray<AFootballer*> Team1Players;

	UPROPERTY() FVector FieldCentreWS = FVector::ZeroVector;
	UPROPERTY() TArray<FVector> BaseFormation_Local;

	// Simple field clamps (world units); tune to your pitch size
	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfLength = 4600.f;   // X extent from centre to goal line

	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfWidth = 3000.f;   // Y extent from centre to sideline
};
