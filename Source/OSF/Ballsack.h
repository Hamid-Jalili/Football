#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ballsack.generated.h"

/** Ball actor with overlap-based possession tracking */
UCLASS()
class OSF_API ABallsack : public AActor
{
	GENERATED_BODY()

public:
	ABallsack();

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Possession")
	class AFootballer* LastTouchingFootballer = nullptr;

	/** 0 TeamA, 1 TeamB, -1 None */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Possession")
	int32 PossessingTeamID = -1;

	UFUNCTION(BlueprintCallable, Category = "Possession")
	int32 GetPossessingTeamID() const { return PossessingTeamID; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Collision")
	class USphereComponent* TouchCollision = nullptr;

	UFUNCTION()
	void OnBallBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
