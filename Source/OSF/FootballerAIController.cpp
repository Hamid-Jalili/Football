#include "FootballerAIController.h"
#include "Ballsack.h"            // <-- MUST be FIRST
#include "OSF.h"

// Fill out your copyright notice in the Description page of Project Settings.


void AFootballerAIController::BeginPlay()
{
    Super::BeginPlay();
}

void AFootballerAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    //AddMovementInput(FVector::ZeroVector);
}