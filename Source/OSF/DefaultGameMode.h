// DefaultGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "FormationRow.h"
#include "DefaultGameMode.generated.h"

class AFootballer;
class AFootballTeam;
class ABallsack;

/**
 * Spawns two teams, assigns roles, spawns the ball, etc.
 * Team size + positions come from an editor DataTable (if set).
 * Now also projects spawn locations to ground to avoid underground spawns.
 */
UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

protected:
	virtual void BeginPlay() override;

	// ----------------------------------------------------
	// Editor classes (set these in BP_GameMode)
	// ----------------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AFootballer> FootballerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AFootballTeam> TeamClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ABallsack> BallClass;

	// ----------------------------------------------------
	// Formation (editor-driven)
	// ----------------------------------------------------
	/** If set, formation (size + positions [+ optional roles]) will be read from here. */
	UPROPERTY(EditAnywhere, Category = "Formation")
	UDataTable* FormationTable = nullptr;

	/** Effective squad size per team. If no table is set, we fall back to default 11. */
	UPROPERTY(EditAnywhere, Category = "Formation", meta = (ClampMin = "1", UIMin = "1"))
	int32 PlayersPerTeam = 11;

	/** Local-space formation points for Team 0 (Team 1 is mirrored). */
	UPROPERTY() TArray<FVector> BaseFormation_Local;

	/** Optional role per index from the DataTable. */
	UPROPERTY() TArray<EFootballRole> BaseRoles;

	// ----------------------------------------------------
	// Field + grounding
	// ----------------------------------------------------
	/** Pitch centre in world space (auto-found at BeginPlay) */
	UPROPERTY() FVector FieldCentreWS = FVector::ZeroVector;

	/** X extent from centre to goal line */
	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfLength = 4600.f;

	/** Y extent from centre to sideline */
	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfWidth = 3000.f;

	/** Line trace channel used to find the ground surface (your field must Block this). */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	/** How far above centre Z we start the ground ray (uu). */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundTraceUp = 8000.f;

	/** How far below centre Z we trace down (uu). */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundTraceDown = 20000.f;

	/** Small safety lift above the hit point (uu). */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundZOffset = 2.f;

	// ----------------------------------------------------
	// Runtime state
	// ----------------------------------------------------
	UPROPERTY() AFootballTeam* Team0 = nullptr;
	UPROPERTY() AFootballTeam* Team1 = nullptr;

	UPROPERTY() TArray<AFootballer*> Team0Players;
	UPROPERTY() TArray<AFootballer*> Team1Players;

	// ----------------------------------------------------
	// Flow
	// ----------------------------------------------------
	void SpawnTeams();
	void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers);

	// ----------------------------------------------------
	// Helpers
	// ----------------------------------------------------
	/** Default 4-4-2 fallback formation (11) */
	void BuildBaseFormation();

	/** Try to read formation from DataTable; on failure, uses BuildBaseFormation(). */
	void BuildFormationFromTable();

	/** Get local spot / world conversions / clamping / facing */
	FVector FormationLocal(int32 Index) const;
	FVector ToWorld(const FVector& Local) const;
	FVector ClampToField(const FVector& P) const;
	float   TeamHalfAngle(int32 TeamID) const;

	/** Grounding helpers */
	FVector ProjectXYToGround(const FVector& XY) const;        // returns XY with corrected Z at ground+offset
	void    SnapActorToGround(AActor* Actor) const;             // adds capsule half-height if present
};
