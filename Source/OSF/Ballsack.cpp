#include "Ballsack.h"
#include "EngineUtils.h"       // TActorIterator

// Fill out your copyright notice in the Description page of Project Settings.


// Sets default values
ABallsack::ABallsack()
{
	// Set this actor to call Tick() every frame. Turn off if not needed for perf.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ABallsack::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABallsack::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

ABallsack* ABallsack::GetWorldBall(UWorld* World)
{
	if (!World) return nullptr;

	for (TActorIterator<ABallsack> It(World); It; ++It)
	{
		return *It; // first one found
	}
	return nullptr;
}