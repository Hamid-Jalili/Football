#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DefaultGameMode.generated.h"

class UDataTable;
class ABallsack;
class AFootballer;
class AFootballTeam;
struct FFormationRow; // defined in FormationRow.h

UENUM(BlueprintType)
enum class EPlayRole : uint8
{
    HoldLine  UMETA(DisplayName = "Hold Line"),
    Press     UMETA(DisplayName = "Press"),
    Support   UMETA(DisplayName = "Support"),
    Mark      UMETA(DisplayName = "Mark")
};

UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ADefaultGameMode();
    virtual void BeginPlay() override;

    // Possession API (kept)
    UFUNCTION(BlueprintCallable) void NotifyPossession(AFootballer* NewOwner);
    UFUNCTION(BlueprintCallable) void ClearPossession(AFootballer* OldOwner);

protected:
    // ---------- Tunables ----------
    UPROPERTY(EditAnywhere, Category = "Pitch") float HalfLength = 9000.f;
    UPROPERTY(EditAnywhere, Category = "Pitch") float HalfWidth = 6000.f;

    UPROPERTY(EditAnywhere, Category = "Spawning") TSubclassOf<AFootballer>  FootballerClass;
    UPROPERTY(EditAnywhere, Category = "Spawning") TSubclassOf<AFootballTeam> TeamClass;

    UPROPERTY(EditAnywhere, Category = "Ball") TSubclassOf<ABallsack> BallClass;

    UPROPERTY(EditAnywhere, Category = "Formation") UDataTable* FormationTable = nullptr;

    // Local-space formation points (X forward, Y right, origin at field center)
    UPROPERTY(EditAnywhere, Category = "Formation") TArray<FVector> BaseFormation_Local;

    // Optional base roles; raw bytes so we don't fight UHT/Blueprint with enums
    UPROPERTY(EditAnywhere, Category = "Formation") TArray<uint8> BaseRoles;

    UPROPERTY(EditAnywhere, Category = "Formation") int32 PlayersPerTeam = 11;

    // Keep-shape bias (0..1) – higher = tighter lines
    UPROPERTY(EditAnywhere, Category = "AI|Shape") float HomeWeight = 0.65f;

    // Tactics
    UPROPERTY(EditAnywhere, Category = "AI|Tactics") float AdvanceWithBall = 1400.f;
    UPROPERTY(EditAnywhere, Category = "AI|Tactics") float RetreatWithBall = 1200.f;

    // Support triangle offsets
    UPROPERTY(EditAnywhere, Category = "AI|Support") float SupportAhead = 750.f;
    UPROPERTY(EditAnywhere, Category = "AI|Support") float SupportWide = 900.f;

    // Steering
    UPROPERTY(EditAnywhere, Category = "AI|Steering") float ArriveRadius = 260.f;
    UPROPERTY(EditAnywhere, Category = "AI|Steering") float SeparationRadius = 420.f;
    UPROPERTY(EditAnywhere, Category = "AI|Steering") float SeparationStrength = 0.7f;

    // Keeper parameters
    UPROPERTY(EditAnywhere, Category = "AI|Keeper") float KeeperDepth = 900.f;
    UPROPERTY(EditAnywhere, Category = "AI|Keeper") float KeeperChaseRadius = 1200.f;
    UPROPERTY(EditAnywhere, Category = "AI|Keeper") float PenBoxDepth = 2200.f;
    UPROPERTY(EditAnywhere, Category = "AI|Keeper") float PenBoxHalfWidth = 2200.f;

    // Grounding trace
    UPROPERTY(EditAnywhere, Category = "Grounding") float GroundTraceUp = 2000.f;
    UPROPERTY(EditAnywhere, Category = "Grounding") float GroundTraceDown = 4000.f;
    UPROPERTY(EditAnywhere, Category = "Grounding") TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;
    UPROPERTY(EditAnywhere, Category = "Grounding") float GroundZOffset = 0.f;

    // ---------- State ----------
    FVector FieldCentreWS = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "Teams") AFootballTeam* Team0 = nullptr;
    UPROPERTY(VisibleAnywhere, Category = "Teams") AFootballTeam* Team1 = nullptr;

    UPROPERTY(VisibleAnywhere, Category = "Teams") TArray<AFootballer*> Team0Players;
    UPROPERTY(VisibleAnywhere, Category = "Teams") TArray<AFootballer*> Team1Players;

    UPROPERTY(VisibleAnywhere, Category = "Ball") ABallsack* Ball = nullptr;

    // Possession memory
    TWeakObjectPtr<AFootballer> PossessingPlayer;
    int32 PossessingTeamID = -1;

    FTimerHandle ThinkTimer;

    // ---------- Flow ----------
    void SpawnTeams();
    void SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers);

    void BuildFormationFromTable();
    void BuildBaseFormation();

    void Think();
    void DriveTeamAI(const TArray<AFootballer*>& Team, int32 TeamID, bool bAttacking);

    // ---------- Helpers ----------
    FVector FormationLocal(int32 Index) const;
    FVector ToWorld(const FVector& Local) const;
    FVector ClampToField(const FVector& P) const;
    float  TeamHalfAngle(int32 TeamID) const;

    FVector ProjectXYToGround(const FVector& XY) const;
    void    SnapActorToGround(AActor* Actor) const;

    FVector SeekArriveDirection(const FVector& From, const FVector& To) const;
    FVector SeparationVector(AFootballer* Self, const TArray<AFootballer*>& SameTeam) const;

    FVector OwnGoalLocation(int32 TeamID) const;

    void ComputeKeeperTarget(
        int32 TeamID, const FVector& BallLoc,
        FVector& OutGoal, FVector& OutKeeperHome,
        FVector& OutBoxMin, FVector& OutBoxMax) const;
};
