#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballTeam.generated.h"

UCLASS()
class OSF_API AFootballTeam : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TeamID = 0;              // 0 Left/Blue, 1 Right/Red
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float PitchHalfLength = 8000.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TArray<class AFootballer*> Players;

	// Returns a tactical anchor for PlayerIndex (4-4-2) with a small nudge based on ball.
	FVector GetAnchor(bool bAttacking, int32 PlayerIndex, const FVector& BallWorld) const;

private:
	static FVector BaseAnchor(int32 TeamID, float PitchHalf, int32 PlayerIndex, bool bAttack);
};
