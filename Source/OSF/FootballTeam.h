#pragma once

#include "CoreMinimal.h"
// include whatever base header you need here (e.g., #include "GameFramework/Actor.h")
#include "Footballer.h"              // <-- ADD THIS real include
#include "FootballTeam.generated.h"  // <-- MUST be the LAST include in the file



UCLASS()
class OSF_API AFootballTeam : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFootballTeam();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

    TArray<AFootballer*> Footballers;
    
    UFUNCTION(BlueprintCallable, Category=Custom)
    AFootballer* GetClosestFootballerToBall(ABallsack* ball);
	
};
