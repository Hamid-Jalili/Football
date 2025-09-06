// DefaultGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataTable.h"
#include "FormationRow.h"                 // any non-generated includes BEFORE generated.h

// Forward declarations only (NO UCLASS here)
class AFootballer;
class AFootballTeam;
class ABallsack;

// MUST be the last include in this header:
#include "DefaultGameMode.generated.h"

/**
 * Spawns two teams and the ball.
 * Team size + placements can be driven by a DataTable (FFormationRow).
 * Spawns are grounded via line trace; first blue player is possessed.
 */
UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

protected:
	virtual void BeginPlay() override;

	// ---------- Classes ----------
	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AFootballer> FootballerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<AFootballTeam> TeamClass;

	UPROPERTY(EditDefaultsOnly, Category = "Classes")
	TSubclassOf<ABallsack> BallClass;

	// ---------- Formation ----------
	/** If set, formation (size + positions [+ optional roles]) will be read from here. */
	UPROPERTY(EditAnywhere, Category = "Formation")
	UDataTable* FormationTable = nullptr;

	/** Effective squad size per team; when DataTable is set, this auto-matches its row count. */
	UPROPERTY(EditAnywhere, Category = "Formation", meta = (ClampMin = "1", UIMin = "1"))
	int32 PlayersPerTeam = 11;

	/** Local-space formation points for Team 0 (Team 1 is mirrored). */
	UPROPERTY() TArray<FVector> BaseFormation_Local;

	/** Optional per-index role from the DataTable. */
	UPROPERTY() TArray<EFootballRole> BaseRoles;

	// ---------- Field & Grounding ----------
	/** Pitch centre (WS). */
	UPROPERTY() FVector FieldCentreWS = FVector::ZeroVector;

	/** X half-extent (centre to goal line). */
	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfLength = 4600.f;

	/** Y half-extent (centre to sideline). */
	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfWidth = 3000.f;

	/** Trace channel the pitch blocks for grounding. */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	/** Trace start height above centre Z. */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundTraceUp = 8000.f;

	/** Trace depth below centre Z. */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundTraceDown = 20000.f;

	/** Small lift above hit point. */
	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundZOffset = 2.f;

	// ---------- Runtime ----------
	UPROPERTY() AFootballTeam* Team0 = nullptr;
	UPROPERTY() AFootballTeam* Team1 = nullptr;

	UPROPERTY() TArray<AFootballer*> Team0Players;
	UPROPERTY() TArray<AFootballer*> Team1Players;

	// ---------- Flow ----------
	void SpawnTeams();
	void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers);

	// ---------- Helpers ----------
	void BuildBaseFormation();        // fallback 4-4-2
	void BuildFormationFromTable();   // DT -> BaseFormation_Local / BaseRoles

	FVector FormationLocal(int32 Index) const;
	FVector ToWorld(const FVector& Local) const;
	FVector ClampToField(const FVector& P) const;
	float   TeamHalfAngle(int32 TeamID) const;

	// Grounding
	FVector ProjectXYToGround(const FVector& XY) const;    // XY with Z at ground + offset
	void    SnapActorToGround(AActor* Actor) const;         // adds capsule half-height if present
};
