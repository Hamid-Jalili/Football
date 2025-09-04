#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Ballsack.generated.h"

class AFootballer;   // only pointer, so forward declaration is enough

UCLASS()
class OSF_API ABallsack : public AActor
{
	GENERATED_BODY()

public:
	ABallsack();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	int32 PossessingTeamID = -1;

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void SetPossessingFootballer(AFootballer* Footballer);
};
