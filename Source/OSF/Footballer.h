// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
    // Useful typedefs
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
        float Power;
        FVector Direction;
    } PendingActionInfo;

    AFootballer();

    // MARK: - Override Functions
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
    virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

    // MARK: - Authority-set (replicated) properties

    UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
    FVector DesiredMovement;

    UPROPERTY(Replicated)
    float DesiredSprintStrength;

    UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
    ABallsack* Ball;

    /** Player is actively moving to gain/keep the ball */
    UPROPERTY(Replicated)
    bool GoingForPossession;

    /** Local belief if we currently control the ball (between touches). */
    UPROPERTY(Replicated)
    bool ThinksHasPossession;

    /** Waiting to receive a pass */
    UPROPERTY(Replicated)
    bool WaitingForPass;

    /** Anti-spam flag cleared when far enough from ball */
    UPROPERTY(Replicated)
    bool JustKickedBall;

    /** Last time we actually imparted a touch/kick to the ball */
    UPROPERTY(Replicated)
    float LastKickTime;

    /** Controller that owns this footballer when under player control */
    UPROPERTY(Replicated)
    AFootballerController* FootballerController;

    //UPROPERTY(Replicated)
    FootballAttributeInfo FootballAttributes;

    UPROPERTY(Replicated)
    FString DisplayName;

    /** Team reference */
    UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
    AFootballTeam* Team;

    /** AI controller for hot-swap */
    UPROPERTY(BlueprintReadWrite, Category = Custom, Replicated)
    AFootballerAIController* AIController;

    //UPROPERTY(Replicated)
    PendingActionInfo PendingAction;

    UPROPERTY(Replicated, ReplicatedUsing = OnRep_ControlledByPlayer)
    bool ControlledByPlayer;

    /** Cache of teammates */
    UPROPERTY(Replicated)
    TArray<AFootballer*> CachedTeammates;

    UPROPERTY(Replicated)
    bool DoneInitialSetup;

    /** Possession flag: true when we should “carry” (dribble) the ball instead of blasting it */
    UPROPERTY(Replicated)
    bool bHasPossession = false;

    // MARK: - Uncategorized

    UFUNCTION()
    void SetDesiredMovement(FVector movement);

    UFUNCTION(Server, Reliable, WithValidation)
    void Server_SetDesiredMovement(FVector movement);
    virtual void Server_SetDesiredMovement_Implementation(FVector movement);
    virtual bool Server_SetDesiredMovement_Validate(FVector movement);

    UFUNCTION(Server, Reliable, WithValidation)
    void SetDesiredSprintStrength(float strength);
    virtual void SetDesiredSprintStrength_Implementation(float strength);
    virtual bool SetDesiredSprintStrength_Validate(float strength);

    UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
    void Server_GainPlayerControl(AFootballerController* newController);

    UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
    void Server_LosePlayerControl();

    UFUNCTION(BlueprintCallable, Category = Custom)
    bool IsControlledByPlayer();

    UPROPERTY(BlueprintReadWrite, Category = Custom)
    UStaticMeshComponent* PlayerControlIndicator;

    void ShowTargetingIndicator();
    void HideTargetingIndicator();

    UPROPERTY(BlueprintReadWrite, Category = Custom)
    UStaticMeshComponent* TargetingIndicator;

    UFUNCTION(BlueprintCallable, Category = "Queue Actions")
    void ClearPendingAction();

    UFUNCTION(BlueprintCallable, Category = "Queue Actions")
    void SetPendingShot(float power, FVector desiredDirection);

    UFUNCTION(BlueprintCallable, Category = "Queue Actions")
    void SetPendingPass(float power, FVector desiredDirection);

    UFUNCTION(BlueprintCallable, Category = "Control")
    FVector DesiredMovementOrForwardVector();

    UFUNCTION(BlueprintCallable, Category = "Control")
    bool CanKickBall();

    UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
    void KnockBallOn(float deltaSeconds, float strength);

    UFUNCTION(BlueprintCallable, Category = Custom, Server, Reliable, WithValidation)
    void ShootBall(float power, FVector desiredDirection);

    UFUNCTION(Server, BlueprintCallable, Category = Custom, Reliable, WithValidation)
    void PassBall(float power, FVector desiredDirection);

    AFootballer* FindPassTarget(float power, FVector desiredDirection);

    UFUNCTION(BlueprintCallable, Category = "Movement")
    void MoveToBallForKick(FVector desiredEndDirection, float deltaSeconds);

    void MoveToBallForPass(FVector desiredMovement, float deltaSeconds);

    /** Moves based on DesiredMovement without trying to move toward the ball. */
    UFUNCTION(BlueprintCallable, Category = "Movement")
    void FreeMoveDesired();

    UFUNCTION(Server, Reliable, WithValidation)
    void SetGoingForPossession(bool going);

    UFUNCTION(BlueprintCallable, Category = Custom)
    TArray<AFootballer*> Teammates();

    /** Gentle carry of the ball while in possession (no blasting). */
    UFUNCTION(Server, Reliable, WithValidation)
    void DribbleBall(float DeltaSeconds);
    virtual void DribbleBall_Implementation(float DeltaSeconds);
    virtual bool DribbleBall_Validate(float DeltaSeconds) { return true; }

    /** True if local input vector is meaningful */
    bool HasMeaningfulInput() const;

private:
    UFUNCTION()
    void OnRep_ControlledByPlayer();
};
