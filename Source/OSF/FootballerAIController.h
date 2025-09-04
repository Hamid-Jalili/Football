#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FootballerAIController.generated.h"

class AFootballer;

/**
 * Very light AI controller; we keep it minimal to avoid regressions.
 * You can flesh out behavior later—this existence + ctor fix the linker error.
 */
UCLASS()
class OSF_API AFootballerAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFootballerAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	// Weak ref so we don't keep pawns alive
	TWeakObjectPtr<AFootballer> Me;
};
