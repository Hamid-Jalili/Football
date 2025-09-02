#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Footballer.h"                 // UHT needs the real type in headers that use it
#include "DefaultGameMode.generated.h"

class AFootballerAIController;
class APlayerController;

UCLASS()
class OSF_API ADefaultGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADefaultGameMode();

protected:
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

	// Spawn a team in 4-4-2: [0]=GK, [1..4]=DEF, [5..8]=MID, [9..10]=FWD
	void SpawnTeam(int32 TeamIndex, const FVector& HalfBase, bool bHomeTeam);
	FVector FormationOffset(int32 RoleIndex, bool bHomeTeam) const;

	// Pawn/AI classes (we try to find BP_Footballer at runtime)
	UPROPERTY(EditDefaultsOnly, Category = "Teams")
	TSubclassOf<AFootballer> FootballerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Teams")
	TSubclassOf<AFootballerAIController> AIControllerClass;

	// Pitch & spacing
	UPROPERTY(EditDefaultsOnly, Category = "Teams|Pitch")
	float PitchHalfLength = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Teams|Pitch")
	float PitchHalfWidth = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Teams|Spacing")
	float LineDepth = 800.f;   // distance between GK/DEF/MID/FWD lines (toward center line)

	UPROPERTY(EditDefaultsOnly, Category
