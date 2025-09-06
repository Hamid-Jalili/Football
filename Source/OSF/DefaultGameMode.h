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

// Light role tag for debug/intent
UENUM(BlueprintType)
enum class EPlayRole : uint8
{
	Press,
	Mark,
	Support,
	HoldLine
};

/**
 * Spawns two teams and the ball.
 * Formation may come from a DataTable if assigned.
 * Spawns are grounded; first blue player is possessed.
 * Tracks possession and drives simple dynamic AI (attack/defend).
 */
UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

	// Called by controllers/AI when ball possession changes
	UFUNCTION(BlueprintCallable, Category = "Match")
	void NotifyPossession(AFootballer* NewOwner);

	UFUNCTION(BlueprintCallable, Category = "Match")
	void ClearPossession(AFootballer* OldOwner);

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
	UPROPERTY(EditAnywhere, Category = "Formation")
	UDataTable* FormationTable = nullptr;

	UPROPERTY(EditAnywhere, Category = "Formation", meta = (ClampMin = "1", UIMin = "1"))
	int32 PlayersPerTeam = 11;

	UPROPERTY() TArray<FVector>        BaseFormation_Local;
	UPROPERTY() TArray<EFootballRole> BaseRoles;

	// ---------- Field & Grounding ----------
	UPROPERTY() FVector FieldCentreWS = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfLength = 4600.f;

	UPROPERTY(EditAnywhere, Category = "Field")
	float HalfWidth = 3000.f;

	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundTraceUp = 8000.f;

	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundTraceDown = 20000.f;

	UPROPERTY(EditAnywhere, Category = "Field|Grounding")
	float GroundZOffset = 2.f;

	// ---------- Runtime ----------
	UPROPERTY() AFootballTeam* Team0 = nullptr;
	UPROPERTY() AFootballTeam* Team1 = nullptr;

	UPROPERTY() TArray<AFootballer*> Team0Players;
	UPROPERTY() TArray<AFootballer*> Team1Players;

	UPROPERTY() ABallsack* Ball = nullptr;

	// Possession / attack state
	UPROPERTY(VisibleAnywhere, Category = "Match")
	TWeakObjectPtr<AFootballer> PossessingPlayer;

	UPROPERTY(VisibleAnywhere, Category = "Match")
	int32 PossessingTeamID = -1; // -1 = none

	FTimerHandle ThinkTimer; // periodic AI update

	// ---- AI tuning ----
	UPROPERTY(EditAnywhere, Category = "AI")
	float PressDistance = 350.f;          // contain distance to ball

	UPROPERTY(EditAnywhere, Category = "AI")
	float SupportAhead = 420.f;           // how far ahead of ball supporters sit
	UPROPERTY(EditAnywhere, Category = "AI")
	float SupportWide = 520.f;           // how far left/right from ball
	UPROPERTY(EditAnywhere, Category = "AI")
	float AdvanceWithBall = 900.f;        // line advance while attacking

	UPROPERTY(EditAnywhere, Category = "AI")
	float RetreatWithBall = 900.f;        // line drop while defending

	UPROPERTY(EditAnywhere, Category = "AI")
	float ArriveRadius = 220.f;           // slow down when close to target

	UPROPERTY(EditAnywhere, Category = "AI")
	float SeparationRadius = 280.f;       // start repulsion if closer than this
	UPROPERTY(EditAnywhere, Category = "AI")
	float SeparationStrength = 1.5f;      // repulsion weight

	// ---------- Flow ----------
	void SpawnTeams();
	void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers);

	// ---------- Helpers ----------
	void BuildBaseFormation();
	void BuildFormationFromTable();

	FVector FormationLocal(int32 Index) const;
	FVector ToWorld(const FVector& Local) const;
	FVector ClampToField(const FVector& P) const;
	float   TeamHalfAngle(int32 TeamID) const;

	// Grounding
	FVector ProjectXYToGround(const FVector& XY) const;
	void    SnapActorToGround(AActor* Actor) const;

	// AI brain
	void Think(); // called every 0.1s
	void DriveTeamAI(const TArray<AFootballer*>& Team, int32 TeamID, bool bAttacking);

	// Steering utilities
	FVector SeekArriveDirection(const FVector& From, const FVector& To) const;
	FVector SeparationVector(AFootballer* Self, const TArray<AFootballer*>& SameTeam) const;
	FVector OwnGoalLocation(int32 TeamID) const;
};
