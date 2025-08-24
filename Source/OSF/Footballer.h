// Footballer.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Ballsack.h"
#include "Footballer.generated.h"

class AFootballerController;
class AFootballerAIController;
class AFootballTeam;

UCLASS()
class OSF_API AFootballer : public ACharacter
{
	GENERATED_BODY()

public:
	// ----- Types -----
	struct FootballAttributeInfo
	{
		int SprintSpeed = 75; // km/h-ish scale used by project (multiplied to cm/s in BeginPlay)
	};

	enum class PendingActionType : uint8
	{
		FootballerActionNone,
		FootballerActionShot,
		FootballerActionPass,
	};

	struct PendingActionInfo
	{
		PendingActionType Type = PendingActionType::FootballerActionNone;
		float   Power = 0.f;
		FVector Direction = FVector::ZeroVector;
	};

public:
	AFootballer();

	// AActor
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// APawn
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// Rep
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	// ----------------- Replicated state -----------------
	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	FVector DesiredMovement = FVector::ZeroVector;

	UPROPERTY(Replicated)
	float DesiredSprintStrength = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	ABallsack* Ball = nullptr;

	UPROPERTY(Replicated)
	bool GoingForPossession = false;

	UPROPERTY(Replicated)
	bool ThinksHasPossession = false;

	UPROPERTY(Replicated)
	bool WaitingForPass = false;

	UPROPERTY(Replicated)
	bool JustKickedBall = false;

	UPROPERTY(Replicated)
	float LastKickTime = 0.f;

	UPROPERTY(Replicated)
	AFootballerController* FootballerController = nullptr;

	// (Not replicated—static sample)
	FootballAttributeInfo FootballAttributes;

	UPROPERTY(Replicated)
	FString DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	AFootballTeam* Team = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
	AFootballerAIController* AIController = nullptr;

	// (Not replicated intentionally; queued locally)
	PendingActionInfo PendingAction;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_ControlledByPlayer)
	bool ControlledByPlayer = false;

	UPROPERTY(Replicated)
	TArray<AFootballer*> CachedTeammates;

	UPROPERTY(Replicated)
	bool DoneInitialSetup = false;

	// ----------------- Cosmetic helpers -----------------
	UPROPERTY(BlueprintReadWrite, Category = Custom)
	UStaticMeshComponent* PlayerControlIndicator = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = Custom)
	UStaticMeshComponent* TargetingIndicator = nullptr;

	// ----------------- Commands / RPCs -----------------
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
	virtual void Server_GainPlayerControl_Implementation(AFootballerController* NewController);
	virtual bool Server_GainPlayerControl_Validate(AFootballerController* /*NewController*/) { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void Server_LosePlayerControl();
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

	// Ball helpers
	UFUNCTION(BlueprintCallable, Category = "Control")
	FVector DesiredMovementOrForwardVector();

	UFUNCTION(BlueprintCallable, Category = "Control")
	bool CanKickBall();

	// Actions on the ball
	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void KnockBallOn(float DeltaSeconds, float Strength);
	virtual void KnockBallOn_Implementation(float DeltaSeconds, float Strength);
	virtual bool KnockBallOn_Validate(float /*DeltaSeconds*/, float /*Strength*/) { return true; }

	UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
	void ShootBall(float Power, FVector DesiredDirection);
	virtual void ShootBall_Implementation(float Power, FVector DesiredDirection);
	virtual bool ShootBall_Validate(float /*Power*/, FVector /*Dir*/) { return true; }

	UFUNCTION(Server, BlueprintCallable, Category = Custom, Reliable, WithValidation)
	void PassBall(float Power, FVector DesiredDirection);
	virtual void PassBall_Implementation(float Power, FVector DesiredDirection);
	virtual bool PassBall_Validate(float /*Power*/, FVector /*Dir*/) { return true; }

	// Team helpers
	UFUNCTION(BlueprintCallable, Category = Custom)
	TArray<AFootballer*> Teammates();
	AFootballer* FindPassTarget(float Power, FVector DesiredDirection);

	// Movement
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveToBallForKick(FVector DesiredEndDirection, float DeltaSeconds);
	void MoveToBallForPass(FVector DesiredMove, float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "Movement")
	void FreeMoveDesired();

	UFUNCTION(Server, Reliable, WithValidation)
	void SetGoingForPossession(bool bGoing);
	virtual void SetGoingForPossession_Implementation(bool bGoing);
	virtual bool SetGoingForPossession_Validate(bool /*bGoing*/) { return true; }

	// Helpers used in .cpp
	static bool CanTouchNow(const UWorld* World, float LastTouch, float CooldownSeconds = 0.30f);
	static FVector MakeValidDesired(FVector Movement, float Sprint01);

private:
	UFUNCTION()
	void OnRep_ControlledByPlayer();
};
