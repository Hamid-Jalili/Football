#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TeamGameState.generated.h"

class ABallsack;
class AFootballTeam;

UCLASS()
class OSF_API ATeamGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    ATeamGameState();

    // Ball class (BP_Ball) – if not set, we’ll try to load /Game/BP_Ball.BP_Ball_C
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ball")
    TSubclassOf<AActor> BallClass;

    // Which team currently has the ball; -1 = none
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Possession")
    int32 PossessingTeamID = -1;

    // Reset ball to centre spot and clear possession
    UFUNCTION(BlueprintCallable, Category = "Ball")
    void ResetBallToCentre();

    // Handle scoring and reset ball; bRightGoal is kept for your logic
    UFUNCTION(BlueprintCallable, Category = "Goals")
    void HandleGoal(int32 ScoringTeamID, bool bRightGoal);

    // Safe getter for ball location (usable from anywhere)
    UFUNCTION(BlueprintCallable, Category = "Ball")
    static FVector GetBallLocationSafe(const UObject* WorldContext);

protected:
    virtual void BeginPlay() override;
};
