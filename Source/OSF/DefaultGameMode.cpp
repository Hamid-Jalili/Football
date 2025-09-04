// DefaultGameMode.cpp
#include "DefaultGameMode.h"

#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

#include "Footballer.h"
#include "FootballTeam.h"
#include "Ballsack.h"

ADefaultGameMode::ADefaultGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	// Sensible defaults can be set in BP_GameMode
	Team0Players.Reserve(11);
	Team1Players.Reserve(11);
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	BuildBaseFormation();

	// Try to locate a BP_Field actor and use its origin as centre if present
	if (AActor* Field = UGameplayStatics::GetActorOfClass(GetWorld(), AActor::StaticClass()))
	{
		// If you have a specific field class, replace this lookup with that class
		// and optionally read its bounds to set HalfLength/HalfWidth.
		FieldCentreWS = Field->GetActorLocation();
	}
	else
	{
		FieldCentreWS = FVector::ZeroVector;
	}

	SpawnTeams();

	// Spawn ball at centre if a class is assigned and no ball exists
	if (BallClass && UGameplayStatics::GetActorOfClass(GetWorld(), BallClass) == nullptr)
	{
		GetWorld()->SpawnActor<ABallsack>(BallClass, FieldCentreWS + FVector(0, 0, 20.f), FRotator::ZeroRotator);
	}
}

void ADefaultGameMode::SpawnTeams()
{
	if (!TeamClass || !FootballerClass) { return; }

	// Spawn Team 0 and Team 1 managers at centre (they're logical managers)
	Team0 = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FieldCentreWS, FRotator::ZeroRotator);
	Team1 = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FieldCentreWS, FRotator(0.f, 180.f, 0.f));

	if (Team0) Team0->TeamID = 0;
	if (Team1) Team1->TeamID = 1;

	TArray<FVector> T0WS; T0WS.Reserve(11);
	TArray<FVector> T1WS; T1WS.Reserve(11);
	TArray<FRotator> T0Rot; T0Rot.Reserve(11);
	TArray<FRotator> T1Rot; T1Rot.Reserve(11);

	for (int32 i = 0; i < 11; ++i)
	{
		SpawnOne(0, i, Team0, Team0Players);
		SpawnOne(1, i, Team1, Team1Players);

		// Collect for debug draw
		const FVector L = FormationLocal(i);
		const FVector W0 = ClampToField(ToWorld(L));                        // left team
		const FVector W1 = ClampToField(ToWorld(FVector(-L.X, -L.Y, L.Z))); // mirrored for right team

		T0WS.Add(W0);
		T1WS.Add(W1);
		T0Rot.Add(FRotator(0.f, TeamHalfAngle(0), 0.f));
		T1Rot.Add(FRotator(0.f, TeamHalfAngle(1), 0.f));
	}

	if (Team0) Team0->AssignRoles();
	if (Team1) Team1->AssignRoles();

	if (bDrawFormation)
	{
		DrawFormationDebug(T0WS, T0Rot, T1WS, T1Rot);
	}
}

void ADefaultGameMode::SpawnOne(int32 TeamID, int32 Index, AFootballTeam* TeamActor, TArray<AFootballer*>& OutPlayers)
{
	if (!TeamActor || !FootballerClass) return;

	// Local spot (left team reference)
	const FVector L = FormationLocal(Index);

	// Mirror for team 1 (right), keep left for team 0
	const FVector LocalForTeam = (TeamID == 0) ? L : FVector(-L.X, -L.Y, L.Z);

	// Convert to world and clamp to field bounds
	FVector SpawnLoc = ClampToField(ToWorld(LocalForTeam));
	FRotator SpawnRot(0.f, TeamHalfAngle(TeamID), 0.f);

	AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, SpawnRot);
	if (!P) return;

	P->TeamID = TeamID;

	// Register into the team manager (keeps its own list and does its role fallback)
	TeamActor->RegisterPlayer(P, Index);

	OutPlayers.Add(P);
}

void ADefaultGameMode::DrawFormationDebug(const TArray<FVector>& Team0WS, const TArray<FRotator>& Team0Rot,
	const TArray<FVector>& Team1WS, const TArray<FRotator>& Team1Rot) const
{
	UWorld* W = GetWorld();
	if (!W) return;

	const float Radius = 32.f;
	const int32 Segs = 16;

	// Team colors
	const FColor C0 = FColor::Blue;
	const FColor C1 = FColor::Red;

	// Spheres + facing arrows for both teams
	for (int32 i = 0; i < Team0WS.Num(); ++i)
	{
		DrawDebugSphere(W, Team0WS[i], Radius, Segs, C0, false, DebugDuration, 0, 1.5f);
		const FVector Fwd0 = Team0Rot[i].RotateVector(FVector::ForwardVector);
		DrawDebugLine(W, Team0WS[i], Team0WS[i] + Fwd0 * 150.f, C0, false, DebugDuration, 0, 2.f);
	}

	for (int32 i = 0; i < Team1WS.Num(); ++i)
	{
		DrawDebugSphere(W, Team1WS[i], Radius, Segs, C1, false, DebugDuration, 0, 1.5f);
		const FVector Fwd1 = Team1Rot[i].RotateVector(FVector::ForwardVector);
		DrawDebugLine(W, Team1WS[i], Team1WS[i] + Fwd1 * 150.f, C1, false, DebugDuration, 0, 2.f);
	}

	// Centre mark
	DrawDebugSphere(W, FieldCentreWS, 24.f, 12, FColor::White, false, DebugDuration, 0, 1.0f);
}

void ADefaultGameMode::BuildBaseFormation()
{
	BaseFormation_Local.Empty(11);

	// Simple 4-4-2 (GK, 4 DEF, 4 MID, 2 FWD) laid out along +X (toward the right from local origin).
	// Local is *left team reference*; mirroring is handled per team.
	// Units are cm. Spread/tune freely.
	// GK
	BaseFormation_Local.Add(FVector(-HalfLength + 500.f, 0.f, 0.f)); // 0

	// DEF line (slightly ahead of GK)
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

	// FWD line
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
	// Team 0 faces +X (0°), Team 1 faces -X (180°)
	return (TeamID == 0) ? 0.f : 180.f;
}
