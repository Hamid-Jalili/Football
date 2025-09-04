// Footballer.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Footballer.generated.h"

// Forward decls
class UMaterialInterface;
class AFootballTeam;

UENUM(BlueprintType)
enum class EFootballRole : uint8
{
	GK  UMETA(DisplayName = "Goalkeeper"),
	DEF UMETA(DisplayName = "Defender"),
	MID UMETA(DisplayName = "Midfielder"),
	FWD UMETA(DisplayName = "Forward")
};

UCLASS()
class OSF_API AFootballer : public ACharacter
{
	GENERATED_BODY()

public:
	AFootballer();

	/** Team index this player belongs to (0 / 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = -1;

	/** Player role (can be assigned by GameMode/Team). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	EFootballRole PlayerRole = EFootballRole::MID;

	/** “Home” position used by simple tactics/respawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FVector HomeLocation = FVector::ZeroVector;

	/** Back reference to the AFootballTeam actor that owns this player (optional). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	TWeakObjectPtr<AFootballTeam> TeamRef;

	/* =======================
	   Appearance (Team kits)
	   ======================= */

	   /** Which Material slot on this mesh represents the kit (usually 0). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	int32 TeamMaterialSlot = 0;

	/** Material to use for Team 0 (assign in BP_Footballer Details). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	UMaterialInterface* Team0Material = nullptr;

	/** Material to use for Team 1 (assign in BP_Footballer Details). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Appearance")
	UMaterialInterface* Team1Material = nullptr;

	/** Apply the correct kit to this mesh based on TeamID and the configured materials. */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void ApplyTeamMaterial(int32 InTeamID);

	/** Helper that sets TeamID then applies the kit. */
	UFUNCTION(BlueprintCallable, Category = "Team")
	void SetTeamAndApply(int32 InTeamID);

	/* ======= Movement API (kept) ======= */
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void SetDesiredMovement(const FVector& WorldDirection);

	UFUNCTION(BlueprintPure, Category = "Movement")
	FVector GetDesiredMove() const { return DesiredMove; }

	/* ======= Controller shims to satisfy existing calls (no behavior changes) ======= */

	/** Stores sprint strength; safe no-op unless gameplay uses it. */
	UFUNCTION(BlueprintCallable, Category = "Input|Shim")
	void SetDesiredSprintStrength(float Strength);

	/** Stub: logs the intention; real logic can be added later without breaking callers. */
	UFUNCTION(BlueprintCallable, Category = "Input|Shim")
	void ShootBall(float Power, const FVector& Direction);

	/** Stub: logs the intention; real logic can be added later without breaking callers. */
	UFUNCTION(BlueprintCallable, Category = "Input|Shim")
	void PassBall(float Power, const FVector& Direction);

protected:
	/** Where AI/Controller writes its requested movement. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector DesiredMove = FVector::ZeroVector;

	/** Last requested sprint strength from controller (0..1). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	float DesiredSprintStrength = 0.f;
};
