// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"   // AStaticMeshActor
#include "Ballsack.generated.h"

UCLASS()
class OSF_API ABallsack : public AStaticMeshActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ABallsack();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaSeconds) override;

	/** Gets the ball for a given world. Call during/after BeginPlay on a map that has a ball. Can be null otherwise. */
	static ABallsack* GetWorldBall(UWorld* World);
};
