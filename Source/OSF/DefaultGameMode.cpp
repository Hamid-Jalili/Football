#include "DefaultGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

#include "Footballer.h"
#include "FootballTeam.h"
#include "Ballsack.h"

ADefaultGameMode::ADefaultGameMode()
{
	PrimaryActorTick.bCanEverTick = false; // we use a timer for AI
	Team0Players.Reserve(11);
	Team1Players.Reserve(11);
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Formation
	if (FormationTable) { BuildFormationFromTable(); }
	else
	{
		BuildBaseFormation();
		PlayersPerTeam = BaseFormation_Local.Num() > 0 ? BaseFormation_Local.Num() : PlayersPerTeam;
	}

	FieldCentreWS = FVector::ZeroVector;

	SpawnTeams();

	// Ball at centre
	if (BallClass && UGameplayStatics::GetActorOfClass(GetWorld(), BallClass) == nullptr)
	{
		const FVector BallLoc = ProjectXYToGround(FieldCentreWS) + FVector(0, 0, 20.f);
		Ball = GetWorld()->SpawnActor<ABallsack>(BallClass, BallLoc, FRotator::ZeroRotator);
	}
	else
	{
		Ball = Cast<ABallsack>(UGameplayStatics::GetActorOfClass(GetWorld(), BallClass));
	}

	// Possess first blue
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (Team0Players.IsValidIndex(0) && Team0Players[0])
		{
			PC->Possess(Team0Players[0]);
		}

		// Optional: cinematic camera
		if (ACameraActor* LevelCam =
			Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass())))
		{
			PC->SetViewTargetWithBlend(LevelCam, 0.f);
		}
	}

	// Start simple AI updates (10 Hz)
	GetWorldTimerManager().SetTimer(ThinkTimer, this, &ADefaultGameMode::Think, 0.1f, true);
}

// ---------------- Possession API ----------------
void ADefaultGameMode::NotifyPossession(AFootballer* NewOwner)
{
	PossessingPlayer = NewOwner;
	PossessingTeamID = NewOwner ? NewOwner->TeamID : -1;
}

void ADefaultGameMode::ClearPossession(AFootballer* OldOwner)
{
	if (PossessingPlayer.Get() == OldOwner)
	{
		PossessingPlayer = nullptr;
		PossessingTeamID = -1;
	}
}

// ---------------- Spawning ----------------
void ADefaultGameMode::SpawnTeams()
{
	if (!TeamClass || !FootballerClass) { return; }

	Team0 = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FieldCentreWS, FRotator::ZeroRotator);
	Team1 = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FieldCentreWS, FRotator(0.f, 180.f, 0.f));

	if (Team0) Team0->TeamID = 0;
	if (Team1) Team1->TeamID = 1;

	for (int32 i = 0; i < PlayersPerTeam; ++i)
	{
		SpawnOne(0, i, Team0, Team0Players);
		SpawnOne(1, i, Team1, Team1Players);
	}

	if (Team0) Team0->AssignRoles();
	if (Team1) Team1->AssignRoles();
}

void ADefaultGameMode::SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers)
{
	if (!TeamActor || !FootballerClass) return;

	const FVector L = FormationLocal(Index);
	const FVector LocalForTeam = (TeamID == 0) ? L : FVector(-L.X, -L.Y, L.Z);

	const FVector XYWorld = ClampToField(ToWorld(LocalForTeam));
	FVector SpawnLoc = ProjectXYToGround(XYWorld);

	FRotator SpawnRot(0.f, TeamHalfAngle(TeamID), 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, SpawnRot, Params);
	if (!P) return;

	P->TeamID = TeamID;

	if (BaseRoles.IsValidIndex(Index)) { P->PlayerRole = BaseRoles[Index]; }

	SnapActorToGround(P);

	TeamActor->RegisterPlayer(P, Index);
	OutPlayers.Add(P);
}

// ---------------- Formation helpers ----------------
void ADefaultGameMode::BuildFormationFromTable()
{
	BaseFormation_Local.Reset();
	BaseRoles.Reset();

	if (!FormationTable)
	{
		BuildBaseFormation();
		PlayersPerTeam = BaseFormation_Local.Num() > 0 ? BaseFormation_Local.Num() : PlayersPerTeam;
		return;
	}

	TArray<FName> RowNames = FormationTable->GetRowNames();

	struct FRowTmp { int32 Index; FVector Pos; EFootballRole Role; };
	TArray<FRowTmp> Rows; Rows.Reserve(RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FFormationRow* Row = FormationTable->FindRow<FFormationRow>(RowName, TEXT("FormationRead"));
		if (!Row) continue;
		Rows.Add({ Row->Index, Row->LocalPosition, Row->Role });
	}

	Rows.Sort([](const FRowTmp& A, const FRowTmp& B) { return A.Index < B.Index; });

	for (const FRowTmp& R : Rows) { BaseFormation_Local.Add(R.Pos); BaseRoles.Add(R.Role); }

	PlayersPerTeam = BaseFormation_Local.Num();

	if (PlayersPerTeam <= 0)
	{
		BuildBaseFormation();
		PlayersPerTeam = BaseFormation_Local.Num() > 0 ? BaseFormation_Local.Num() : 11;
	}
}

void ADefaultGameMode::BuildBaseFormation()
{
	BaseFormation_Local.Empty(11);

	// GK
	BaseFormation_Local.Add(FVector(-HalfLength + 500.f, 0.f, 0.f)); // 0

	// DEF line
	const float DefX = -HalfLength * 0.5f;
	BaseFormation_Local.Add(FVector(DefX, -1400.f, 0.f)); // 1
	BaseFormation_Local.Add(FVector(DefX, -400.f, 0.f)); // 2
	BaseFormation_Local.Add(FVector(DefX, 400.f, 0.f)); // 3
	BaseFormation_Local.Add(FVector(DefX, 1400.f, 0.f)); // 4

	// MID line
	const float MidX = 0.f;
	BaseFormation_Local.Add(FVector(MidX, -1600.f, 0.f)); // 5
	BaseFormation_Local.Add(FVector(MidX, -500.f, 0.f)); // 6
	BaseFormation_Local.Add(FVector(MidX, 500.f, 0.f)); // 7
	BaseFormation_Local.Add(FVector(MidX, 1600.f, 0.f)); // 8

	// FWDs
	const float FwdX = HalfLength * 0.35f;
	BaseFormation_Local.Add(FVector(FwdX, -800.f, 0.f)); // 9
	BaseFormation_Local.Add(FVector(FwdX, 800.f, 0.f)); // 10
}

FVector ADefaultGameMode::FormationLocal(int32 Index) const
{
	return BaseFormation_Local.IsValidIndex(Index) ? BaseFormation_Local[Index] : FVector::ZeroVector;
}

FVector ADefaultGameMode::ToWorld(const FVector& Local) const
{
	return FieldCentreWS + Local;
}

FVector ADefaultGameMode::ClampToField(const FVector& P) const
{
	FVector Out = P;
	Out.X = FMath::Clamp(Out.X, FieldCentreWS.X - HalfLength, FieldCentreWS.X + HalfLength);
	Out.Y = FMath::Clamp(Out.Y, FieldCentreWS.Y - HalfWidth, FieldCentreWS.Y + HalfWidth);
	return Out;
}

float ADefaultGameMode::TeamHalfAngle(int32 TeamID) const
{
	return (TeamID == 0) ? 0.f : 180.f;
}

// ---------------- Grounding ----------------
FVector ADefaultGameMode::ProjectXYToGround(const FVector& XY) const
{
	const FVector Start(XY.X, XY.Y, FieldCentreWS.Z + GroundTraceUp);
	const FVector End(XY.X, XY.Y, FieldCentreWS.Z - GroundTraceDown);

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ProjectXYToGround), false);
	Params.bReturnPhysicalMaterial = false;
	Params.AddIgnoredActor(this);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, GroundTraceChannel, Params);
	const float Z = bHit ? Hit.Location.Z : FieldCentreWS.Z;

	return FVector(XY.X, XY.Y, Z + GroundZOffset);
}

void ADefaultGameMode::SnapActorToGround(AActor* Actor) const
{
	if (!Actor) return;

	FVector L = Actor->GetActorLocation();
	L = ProjectXYToGround(L);

	if (UCapsuleComponent* Cap = Actor->FindComponentByClass<UCapsuleComponent>())
	{
		L.Z += Cap->GetScaledCapsuleHalfHeight();
	}

	Actor->SetActorLocation(L, false, nullptr, ETeleportType::TeleportPhysics);
}

// ---------------- Simple AI brain ----------------
void ADefaultGameMode::Think()
{
	// Decide who is attacking:
	//  - If someone holds the ball, that team attacks
	//  - Otherwise the team whose player is closest to the ball
	int32 AttackingTeam = PossessingTeamID;

	if (AttackingTeam < 0)
	{
		AActor* BallActor = Ball ? (AActor*)Ball : UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass());
		const FVector BallLoc = BallActor ? BallActor->GetActorLocation() : FieldCentreWS;

		auto MinDistToBall = [&](const TArray<AFootballer*>& Team) -> float
			{
				float Best = TNumericLimits<float>::Max();
				for (AFootballer* P : Team)
				{
					if (!IsValid(P)) continue;
					Best = FMath::Min(Best, FVector::DistSquared(P->GetActorLocation(), BallLoc));
				}
				return Best;
			};

		const float D0 = MinDistToBall(Team0Players);
		const float D1 = MinDistToBall(Team1Players);
		AttackingTeam = (D0 <= D1) ? 0 : 1;
	}

	DriveTeamAI(Team0Players, 0, AttackingTeam == 0);
	DriveTeamAI(Team1Players, 1, AttackingTeam == 1);
}

void ADefaultGameMode::DriveTeamAI(const TArray<AFootballer*>& Team, int32 TeamID, bool bAttacking)
{
	AActor* BallActor = Ball ? (AActor*)Ball : UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass());
	const FVector BallLoc = BallActor ? BallActor->GetActorLocation() : FieldCentreWS;

	// progress along X from -HalfLength..+HalfLength
	const float BallPhase = FMath::Clamp((BallLoc.X - FieldCentreWS.X) / HalfLength, -1.f, +1.f);
	const float Dir = (TeamID == 0) ? +1.f : -1.f; // 0 attacks +X, 1 attacks -X

	for (int32 i = 0; i < Team.Num(); ++i)
	{
		AFootballer* P = Team[i];
		if (!IsValid(P)) continue;

		// Skip if this player is human-controlled
		if (P->GetController() && P->GetController()->IsPlayerController())
			continue;

		// Base slot
		FVector Slot = FormationLocal(i);
		if (TeamID == 1) Slot = FVector(-Slot.X, -Slot.Y, Slot.Z);
		FVector Target = ToWorld(Slot);

		// Attack/defend shift on X
		if (bAttacking)
		{
			// push up with the ball, capped
			const float Advance = 800.f * BallPhase * Dir;
			Target.X += Advance;

			// two closest supporters hover near the ball
			const float DistToBall = FVector::Dist2D(P->GetActorLocation(), BallLoc);
			if (DistToBall < 2200.f)
			{
				// fan out around ball on Y
				const float Side = (i % 2 == 0) ? +1.f : -1.f;
				Target = BallLoc + FVector(-300.f * Dir, Side * 450.f, 0.f);
			}
		}
		else
		{
			// drop back between ball and own goal
			const float Retreat = 900.f * (0.2f + 0.8f * FMath::Abs(BallPhase)) * Dir;
			Target.X -= Retreat;

			// pinch toward ball on Y slightly
			Target.Y = FMath::Lerp(Target.Y, BallLoc.Y, 0.25f);
		}

		// Clamp to pitch and ground (defensive safety)
		Target = ClampToField(Target);
		Target = ProjectXYToGround(Target);

		const FVector ToT = (Target - P->GetActorLocation());
		const FVector DirMove = ToT.GetSafeNormal();

		P->SetDesiredMovement(DirMove);
		P->SetDesiredSprintStrength(ToT.Size() > 600.f ? 1.f : 0.f);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// debug line to target
		DrawDebugDirectionalArrow(GetWorld(), P->GetActorLocation(), Target, 50.f,
			bAttacking ? FColor::Yellow : FColor::Blue, false, 0.12f, 0, 2.f);
#endif
	}
}
