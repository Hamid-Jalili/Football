#include "FootballerController.h"
#include "Footballer.h"

#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "InputCoreTypes.h"

AFootballerController::AFootballerController()
{
	PrimaryActorTick.bCanEverTick = true; // for small HUD & smooth rotation
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AFootballerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly{});

	// Cache immediately & once more after GM spawn
	CacheRefs();
	FTimerHandle Th;
	GetWorldTimerManager().SetTimer(Th, this, &AFootballerController::CacheRefs, InitialCacheDelay, false);

	Screen(TEXT("PC BeginPlay"));
}

void AFootballerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	check(InputComponent);

	// Axes
	InputComponent->BindAxis("MoveForward", this, &AFootballerController::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AFootballerController::MoveRight);

	// Actions (kept from your project)
	InputComponent->BindAction("Sprint", IE_Pressed, this, &AFootballerController::SprintPressed);
	InputComponent->BindAction("Sprint", IE_Released, this, &AFootballerController::SprintReleased);
	InputComponent->BindAction("Shoot", IE_Pressed, this, &AFootballerController::ShootPressed);
	InputComponent->BindAction("Pass", IE_Pressed, this, &AFootballerController::PassPressed);

	// Switching (+ your existing "SwitchPlayer" if you had one)
	InputComponent->BindAction("SwitchToClosest", IE_Pressed, this, &AFootballerController::SwitchToClosest);
	InputComponent->BindAction("SwitchPlayer", IE_Pressed, this, &AFootballerController::SwitchToClosest);
	InputComponent->BindAction("CycleNext", IE_Pressed, this, &AFootballerController::CycleNext);
	InputComponent->BindAction("CyclePrev", IE_Pressed, this, &AFootballerController::CyclePrev);

	// Raw key probe to prove input reach
	InputComponent->BindKey(EKeys::D, IE_Pressed, this, &AFootballerController::DebugKeyD);

	Screen(TEXT("PC Input bound"));
}

void AFootballerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Tiny HUD every 0.5s
	if (!bDebugInput) return;
	const double Now = GetWorld()->TimeSeconds;
	if (Now - LastTickHudTime < 0.5) return;
	LastTickHudTime = Now;

	AFootballer* Me = GetControlledFootballer();
	FString PawnName = GetNameSafe(Me);
	bool bHasCharMove = false;
	float Speed = 0.f;

	if (ACharacter* C = Cast<ACharacter>(Me))
	{
		if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
		{
			bHasCharMove = true;
			Speed = C->GetVelocity().Size();
		}
	}

	Screen(FString::Printf(TEXT("Pawn:%s  CharMove:%s  Speed:%.1f  Axis F/R: %.2f / %.2f"),
		*PawnName, bHasCharMove ? TEXT("Yes") : TEXT("No"), Speed, AxisForward, AxisRight), FColor::White, 0.6f);
}

void AFootballerController::Screen(const FString& Msg, const FColor& C, float T) const
{
	if (bDebugInput && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, T, C, Msg);
	}
}

void AFootballerController::DebugKeyD()
{
	Screen(TEXT("Key D pressed (raw)"), FColor::Orange, 0.4f);
}

AFootballer* AFootballerController::GetControlledFootballer() const
{
	return Cast<AFootballer>(GetPawn());
}

void AFootballerController::GetCameraBasis(FVector& OutForward, FVector& OutRight) const
{
	const FRotator CamRot = PlayerCameraManager ? PlayerCameraManager->GetCameraRotation() : GetControlRotation();
	FVector Fwd = FRotationMatrix(CamRot).GetUnitAxis(EAxis::X);
	FVector Rt = FRotationMatrix(CamRot).GetUnitAxis(EAxis::Y);
	Fwd.Z = 0.f; Rt.Z = 0.f;
	Fwd.Normalize(); Rt.Normalize();
	OutForward = Fwd; OutRight = Rt;
}

void AFootballerController::ApplyDesiredMovement()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		FVector Fwd, Rt; GetCameraBasis(Fwd, Rt);
		const FVector Desired = (Fwd * AxisForward) + (Rt * AxisRight);

		// 1) keep your hook
		Me->SetDesiredMovement(Desired);

		// 2) Character fallback for guaranteed motion during tests
		if (!Desired.IsNearlyZero())
		{
			const FVector Dir = Desired.GetSafeNormal();
			Me->AddMovementInput(Dir, 1.0f);
		}

		// For non-Character pawns, rotate toward the desired direction
		if (!Desired.IsNearlyZero() && !Cast<ACharacter>(Me))
		{
			const float Delta = GetWorld()->GetDeltaSeconds();
			const FRotator TargetRot(0.f, Desired.Rotation().Yaw, 0.f);
			const FRotator NewRot = FMath::RInterpTo(Me->GetActorRotation(), TargetRot, Delta, 9.f);
			Me->SetActorRotation(NewRot);
		}

		// Visualize desired
		DrawDebugDirectionalArrow(GetWorld(),
			Me->GetActorLocation(),
			Me->GetActorLocation() + Desired.GetClampedToMaxSize(300.f),
			25.f, FColor::Cyan, false, 0.1f, 0, 2.f);

		Screen(FString::Printf(TEXT("Axis F:%.2f R:%.2f | Pawn:%s"),
			AxisForward, AxisRight, *GetNameSafe(Me)), FColor::Cyan, 0.2f);
	}
	else
	{
		Screen(TEXT("No Pawn possessed"), FColor::Red, 0.5f);
	}
}

void AFootballerController::MoveForward(float Value)
{
	AxisForward = Value;
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER) Screen(FString::Printf(TEXT("MoveForward %.2f"), Value), FColor::Green, 0.2f);
	ApplyDesiredMovement();
}
void AFootballerController::MoveRight(float Value)
{
	AxisRight = Value;
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER) Screen(FString::Printf(TEXT("MoveRight %.2f"), Value), FColor::Green, 0.2f);
	ApplyDesiredMovement();
}

void AFootballerController::SprintPressed()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->SetDesiredSprintStrength(1.f);
		Screen(TEXT("Sprint ON"));
	}
}
void AFootballerController::SprintReleased()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->SetDesiredSprintStrength(0.f);
		Screen(TEXT("Sprint OFF"));
	}
}
void AFootballerController::ShootPressed()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->ShootBall(1.f, Me->GetActorForwardVector());
		Screen(TEXT("Shoot"));
	}
}
void AFootballerController::PassPressed()
{
	if (AFootballer* Me = GetControlledFootballer())
	{
		Me->PassBall(0.75f, Me->GetActorForwardVector());
		Screen(TEXT("Pass"));
	}
}

void AFootballerController::CacheRefs()
{
	TeamMates.Reset();
	for (TActorIterator<AFootballer> It(GetWorld()); It; ++It)
	{
		if (It->TeamID == ControlledTeamID)
		{
			TeamMates.Add(*It);
		}
	}

	BallActor = FindBallActor();

	Screen(FString::Printf(TEXT("Cached: Teammates=%d, Ball=%s"),
		TeamMates.Num(), *GetNameSafe(BallActor)));

	AFootballer* Start = FindClosestToBall();
	if (!Start && TeamMates.Num() > 0) Start = TeamMates[0];
	PossessFootballer(Start);
}

AActor* AFootballerController::FindBallActor() const
{
	AActor* Best = nullptr; float BestScore = -FLT_MAX;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const FString N = It->GetName();
		if (N.Contains(TEXT("Ball"), ESearchCase::IgnoreCase))
		{
			const float Score = -N.Len();
			if (Score > BestScore) { BestScore = Score; Best = *It; }
		}
	}
	return Best;
}

AFootballer* AFootballerController::FindClosestToBall() const
{
	if (!BallActor) return nullptr;

	const FVector BallLoc = BallActor->GetActorLocation();
	float BestDistSq = TNumericLimits<float>::Max();
	AFootballer* Best = nullptr;

	for (AFootballer* P : TeamMates)
	{
		if (!IsValid(P)) continue;
		const float D = FVector::DistSquared(P->GetActorLocation(), BallLoc);
		if (D < BestDistSq) { BestDistSq = D; Best = P; }
	}
	return Best;
}

AFootballer* AFootballerController::FindCycle(bool bNext) const
{
	if (TeamMates.Num() == 0) return nullptr;

	const FVector Ref = BallActor ? BallActor->GetActorLocation()
		: (GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector);

	TArray<AFootballer*> Sorted = TeamMates;
	Sorted.Sort([&](AFootballer& A, AFootballer& B)
		{
			return FVector::DistSquared(A.GetActorLocation(), Ref)
				< FVector::DistSquared(B.GetActorLocation(), Ref);
		});

	int32 CurIdx = Sorted.IndexOfByKey(CurrentControlled);
	if (CurIdx == INDEX_NONE) CurIdx = 0;
	const int32 Delta = bNext ? +1 : -1;
	const int32 NewIdx = (CurIdx + Delta + Sorted.Num()) % Sorted.Num();
	return Sorted[NewIdx];
}

void AFootballerController::SwitchToClosest()
{
	if (GetWorld()->TimeSeconds - LastSwitchTime < SwitchCooldown) return;
	LastSwitchTime = GetWorld()->TimeSeconds;

	if (AFootballer* Closest = FindClosestToBall())
	{
		if (Closest != CurrentControlled)
		{
			Screen(FString::Printf(TEXT("Switch → %s"), *GetNameSafe(Closest)));
			PossessFootballer(Closest);
		}
	}
	else
	{
		Screen(TEXT("Switch: no closest (no ball or teammates)"), FColor::Red);
	}
}

void AFootballerController::CycleNext()
{
	if (GetWorld()->TimeSeconds - LastSwitchTime < SwitchCooldown) return;
	LastSwitchTime = GetWorld()->TimeSeconds;

	if (AFootballer* Next = FindCycle(true))
	{
		Screen(FString::Printf(TEXT("CycleNext → %s"), *GetNameSafe(Next)));
		PossessFootballer(Next);
	}
}

void AFootballerController::CyclePrev()
{
	if (GetWorld()->TimeSeconds - LastSwitchTime < SwitchCooldown) return;
	LastSwitchTime = GetWorld()->TimeSeconds;

	if (AFootballer* Prev = FindCycle(false))
	{
		Screen(FString::Printf(TEXT("CyclePrev → %s"), *GetNameSafe(Prev)));
		PossessFootballer(Prev);
	}
}

void AFootballerController::ApplyPlayerFacingIfCharacter(AFootballer* P)
{
	if (!P) return;

	if (ACharacter* C = Cast<ACharacter>(P))
	{
		if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
		{
			// Save once
			if (!SavedOrient.Contains(P))
			{
				FOrientSnapshot Snap;
				Snap.bUseControllerRotationYaw = C->bUseControllerRotationYaw;
				Snap.bOrientRotationToMovement = Move->bOrientRotationToMovement;
				Snap.RotationRate = Move->RotationRate;
				SavedOrient.Add(P, Snap);
			}

			// Player-facing: rotate to movement
			C->bUseControllerRotationYaw = false;
			Move->bUseControllerDesiredRotation = false;
			Move->bOrientRotationToMovement = true;
			Move->RotationRate = FRotator(0.f, 720.f, 0.f);
		}
	}
}

void AFootballerController::PossessFootballer(AFootballer* P)
{
	if (!P) { Screen(TEXT("Possess: nullptr"), FColor::Red); return; }

	// Restore previous pawn’s orientation & give it AI back
	if (CurrentControlled && CurrentControlled != P)
	{
		if (ACharacter* PrevC = Cast<ACharacter>(CurrentControlled))
		{
			if (UCharacterMovementComponent* PrevMove = PrevC->GetCharacterMovement())
			{
				if (FOrientSnapshot* Snap = SavedOrient.Find(CurrentControlled))
				{
					PrevC->bUseControllerRotationYaw = Snap->bUseControllerRotationYaw;
					PrevMove->bOrientRotationToMovement = Snap->bOrientRotationToMovement;
					PrevMove->RotationRate = Snap->RotationRate;
				}
			}
		}

		UnPossess();
		CurrentControlled->SpawnDefaultController();
	}

	CurrentControlled = P;
	Possess(P);

	// Configure facing for the pawn we now control
	ApplyPlayerFacingIfCharacter(P);

	Screen(FString::Printf(TEXT("Possessed %s"), *GetNameSafe(P)), FColor::Yellow);

	// Re-apply current input immediately so there’s no dead frame
	ApplyDesiredMovement();
}
