#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ballsack.generated.h"

UCLASS()
class OSF_API ABallsack : public AActor
{
    GENERATED_BODY()
public:
    ABallsack();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Possession")
    class AFootballer* LastTouchingFootballer = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Possession")
    int32 PossessingTeamID = -1;

    UFUNCTION(BlueprintCallable, Category = "Possession")
    int32 GetPossessingTeamID() const { return PossessingTeamID; }

    /** Teleport to centre and zero all velocities (no drift) */
    void ResetToCenter(const FVector& CentreWorld);

private:
    /** Root collision/physics sphere */
    UPROPERTY(VisibleAnywhere, Category = "Collision")
    class USphereComponent* TouchCollision = nullptr;

    UFUNCTION()
    void OnBallBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );
};
