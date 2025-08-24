// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameFramework/Character.h"
#include "Ballsack.h"
#include "OSF.h"

class AFootballerController;
class AFootballerAIController;
class AFootballTeam;

#include "Footballer.generated.h"

UCLASS()
class OSF_API AFootballer : public ACharacter
{
	GENERATED_BODY()

public:
	// --- Useful typedefs ------------------------------------------------------
	typedef struct _FootballAttributeInfo {
		int SprintSpeed;
	} FootballAttributeInfo;

	typedef enum class _PendingActionType {
		FootballerActionNone,
		FootballerActionShot,
		FootballerActionPass,
	} PendingActionType;

	typedef struct _PendingActionInfo {
		PendingActionType Type;
		float   Power;
		FVector Direction;
	} PendingActionInfo;

	AFootballer();

	// --- AActor ---------------------------------------------------------------
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	// --- Authority-set (replicated) properties --------------------------------
	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	FVector DesiredMovement;

	UPROPERTY(Replicated)
	float DesiredSprintStrength;

	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	ABallsack* Ball;

	UPROPERTY(Replicated)
	bool GoingForPossession;

	UPROPERTY(Replicated)
	bool ThinksHasPossession;

	UPROPERTY(Replicated)
	bool WaitingForPass;

	UPROPERTY(Replicated)
	bool JustKickedBall;

	UPROPERTY(Replicated)
	float LastKickTime;

	UPROPERTY(Replicated)
	AFootballerController* FootballerController;

	FootballAttributeInfo FootballAttributes;

	UPROPERTY(Replicated)
	FString DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	AFootballTeam* Team;

	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	AFootballerAIController* AIController;

	PendingActionInfo PendingAction;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_ControlledByPlayer)
	bool ControlledByPlayer;

	UPROPERTY(Replicated)
	TArray<AFootballer*> CachedTeammates;

	UPROPERTY(Replicated)
	bool DoneInitialSetup;

	// --- Visual indicators ----------------------------------------------------
	UPROPERTY(BlueprintReadWrite, Category = Custom)
	UStaticMeshComponent* PlayerControlIndicator;

	UPROPERTY(BlueprintReadWrite, Category = Custom)
	UStaticMeshComponent* TargetingIndicator;

	// --- Public API -----------------------------------------------------------
	UFUNCTION()
	void SetDesiredMovement(FVector Movement);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetDesiredMovement(FVector Movement);
	virtual void Server_SetDesiredMovement_Implementation(FVector Movement);
	virtual bool Server_SetDesiredMovement_Validate(FVector Movement) { return true; }

	UFUNCTION(Server, Reliable, WithValidation)
	void SetDesiredSprintStrength(float Strength);
	virtual void SetDesiredSprintStrength_Implementation(float Strength);
	virtual bool SetDesiredSprintStrength_Validate(float Strength) { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void Server_GainPlayerControl(AFootballerController* NewController);
	// NOTE: declare only (no inline body) – defined in .cpp
	virtual void Server_GainPlayerControl_Implementation(AFootballerController* NewController);
	virtual bool Server_GainPlayerControl_Validate(AFootballerController* /*NewController*/) { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void Server_LosePlayerControl();
	// NOTE: declare only (no inline body) – defined in .cpp
	virtual void Server_LosePlayerControl_Implementation();
	virtual bool Server_LosePlayerControl_Validate() { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom)
	bool IsControlledByPlayer() { return ControlledByPlayer; }

	void ShowTargetingIndicator();
	void HideTargetingIndicator();

	// Queue actions
	UFUNCTION(BlueprintCallable, Category = "Queue Actions")
	void ClearPendingAction();

	UFUNCTION(BlueprintCallable, Category = "Queue Actions")
	void SetPendingShot(float Power, FVector DesiredDirection);

	UFUNCTION(BlueprintCallable, Category = "Queue Actions")
	void SetPendingPass(float Power, FVector DesiredDirection);

	// Control helpers
	UFUNCTION(BlueprintCallable, Category = "Control")
	FVector DesiredMovementOrForwardVector();

	UFUNCTION(BlueprintCallable, Category = "Control")
	bool CanKickBall();

	// Ball interactions
	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void KnockBallOn(float DeltaSeconds, float Strength);
	virtual void KnockBallOn_Implementation(float DeltaSeconds, float Strength);
	virtual bool KnockBallOn_Validate(float /*DeltaSeconds*/, float /*Strength*/) { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void ShootBall(float Power, FVector DesiredDirection);
	virtual void ShootBall_Implementation(float Power, FVector DesiredDirection);
	virtual bool ShootBall_Validate(float /*Power*/, FVector /*DesiredDirection*/) { return true; }

	UFUNCTION(Server, BlueprintCallable, Category = Custom, Reliable, WithValidation)
	void PassBall(float Power, FVector DesiredDirection);
	virtual void PassBall_Implementation(float Power, FVector DesiredDirection);
	virtual bool PassBall_Validate(float /*Power*/, FVector /*DesiredDirection*/) { return true; }

	AFootballer* FindPassTarget(float Power, FVector DesiredDirection);

	// Movement styles
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveToBallForKick(FVector DesiredEndDirection, float DeltaSeconds);

	void MoveToBallForPass(FVector DesiredMovement, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void FreeMoveDesired();

	UFUNCTION(Server, Reliable, WithValidation)
	void SetGoingForPossession(bool bGoing);
	virtual void SetGoingForPossession_Implementation(bool bGoing);
	virtual bool SetGoingForPossession_Validate(bool /*bGoing*/) { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom)
	TArray<AFootballer*> Teammates();

	// Small helpers exposed for both sides
	static bool  CanTouchNow(const UWorld* World, float LastTouch, float CooldownSeconds);
	static FVector MakeValidDesired(FVector Movement, float Sprint01);

private:
	UFUNCTION()
	void OnRep_ControlledByPlayer();
};
