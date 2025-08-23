#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Ballsack.generated.h"

UCLASS()
class OSF_API ABallsack : public AActor
{
    GENERATED_BODY()

public:
    ABallsack();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaSeconds) override;

    // Ball Mesh
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
    UStaticMeshComponent* BallMesh;
};
