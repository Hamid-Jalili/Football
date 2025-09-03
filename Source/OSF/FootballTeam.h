#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballTeam.generated.h"

UENUM(BlueprintType)
enum class ETeamState : uint8
{
	Attack,
	Defence
};

UCLASS()
class OSF_API AFootballTeam : public AActor
{
	GENERATED_BODY()

public:
	AFootballTeam();

	/** 0 = Team A, 1 = Team B */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	int32 TeamID = 0;

	/** Players belonging to this team (filled by GameMode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	TArray<class AFootballer*> Players;

	/** Current tactical state */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	ETeamState TeamState = ETeamState::Defence;

	/** Update state */
	void SetTeamState(ETeamState NewState);

	/** Anchor for player index (current state) */
	FVector GetAnchorLocation(int32 PlayerIndex) const;

	/** Direct anchors */
	FVector GetAttackAnchor(int32 PlayerIndex) const;
	FVector GetDefenceAnchor(int32 PlayerIndex) const;

protected:
	virtual void BeginPlay() override;

private:
	TArray<FVector> AttackFormation;
	TArray<FVector> DefenceFormation;

	void InitializeFormations();
};
