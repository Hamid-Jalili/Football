#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Footballer.generated.h"

class AFootballTeam;

UCLASS()
class OSF_API AFootballer : public ACharacter
{
	GENERATED_BODY()

public:
	AFootballer();

	// ---- Team / identity ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	AFootballTeam* Team = nullptr;

	UFUNCTION(BlueprintCallable, Category = "Team")
	FORCEINLINE void SetTeamIndex(int32 InIndex) { TeamIndex = InIndex; }

	UFUNCTION(BlueprintCallable, Category = "Team")
	FORCEINLINE int32 GetTeamIndex() const { return TeamIndex; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Team")
	bool bIsGoalkeeper = false;

	UFUNCTION(BlueprintCallable, Category = "Team")
	FORCEINLINE bool IsGoalkeeper() const { return bIsGoalkeeper; }

	UFUNCTION(BlueprintCallable, Category = "Team")
	FORCEINLINE void SetIsGoalkeeper(bool bNew) { bIsGoalkeeper = bNew; }

	// ---- Ball possession ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ball")
	bool bHasBall = false;

	// ---- Movement tuning ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed = 450.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed = 800.f;

	// ---- Input/AI control API ----
	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredMovement(const FVector& MovementWorld);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredMovement2D(const FVector2D& Movement2D);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SetDesiredSprintStrength(float InStrength);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void ShootBall(float Strength, const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void PassBall(float Strength, const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = "Ball")
	void KnockBallOn(float ParamA, float ParamB);

	// RPCs for multiplayer control transfer
	UFUNCTION(Server, Reliable)
	void Server_LosePlayerControl();

	UFUNCTION(Server, Reliable)
	void Server_GainPlayerControl(AController* NewController);

	// ---- Home spot (for AI formation holding) ----
	UFUNCTION(BlueprintCallable, Category = "AI")
	FORCEINLINE void SetHomeSpot(const FVector& InLoc) { HomeSpot = InLoc; }

	UFUNCTION(BlueprintCallable, Category = "AI")
	FORCEINLINE FVector GetHomeSpot() const { return HomeSpot; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Team")
	int32 TeamIndex = -1;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	FVector DesiredMoveWorld = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	float DesiredSprint = 0.f;

	UPROPERTY(VisibleAnywhere, Category = "AI")
	FVector HomeSpot = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (AllowPrivateAccess = "tru
