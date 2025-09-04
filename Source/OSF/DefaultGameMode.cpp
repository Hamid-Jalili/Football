// DefaultGameMode.cpp
#include "DefaultGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"           // NEW: to find your placed camera

#include "Footballer.h"
#include "FootballTeam.h"
#include "Ballsack.h"

ADefaultGameMode::ADefaultGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	Team0Players.Reserve(11);
	Team1Players.Reserve(11);
}

void ADefaultGameMode::BeginPlay()
{
	Super::BeginPlay();

	BuildBaseFormation();

	// Use field actor origin if you have a dedicated field actor, otherwise world origin.
	if (AActor* Field = UGameplayStatics::GetActorOfClass(GetWorld(), AActor::StaticClass()))
	{
		FieldCentreWS = Field->GetActorLocation();
	}
	else
	{
		FieldCentreWS = FVector::ZeroVector;
	}

	SpawnTeams();

	// Spawn ball at centre if needed
	if (BallClass && UGameplayStatics::GetActorOfClass(GetWorld(), BallClass) == nullptr)
	{
		GetWorld()->SpawnActor<ABallsack>(BallClass, FieldCentreWS + FVector(0, 0, 20.f), FRotator::ZeroRotator);
	}

	// ----------------------------
	// VIEW + INPUT HOOKUP
	// ----------------------------
	// Give local player controller possession of Team0’s first player (for input),
	// but set view to the placed camera so the screen looks correct.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		// Prefer Team0’s first pawn for input
		if (Team0Players.Num() > 0 && Team0Players[0])
		{
			PC->Possess(Team0Players[0]);  // input goes to this pawn
		}

		// Keep the view from your placed level camera (e.g., BP_BallCameraActor)
		if (ACameraActor* LevelCam =
			Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass())))
		{
			PC->SetViewTargetWithBlend(LevelCam, 0.f);
		}
	}
}

void ADefaultGameMode::SpawnTeams()
{
	if (!TeamClass || !FootballerClass) { return; }

	Team0 = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FieldCentreWS, FRotator::ZeroRotator);
	Team1 = GetWorld()->SpawnActor<AFootballTeam>(TeamClass, FieldCentreWS, FRotator(0.f, 180.f, 0.f));

	if (Team0) Team0->TeamID = 0;
	if (Team1) Team1->TeamID = 1;

	for (int32 i = 0; i < 11; ++i)
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

	FVector  SpawnLoc = ClampToField(ToWorld(LocalForTeam));
	FRotator SpawnRot(0.f, TeamHalfAngle(TeamID), 0.f);

	AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, SpawnRot);
	if (!P) return;

	P->TeamID = TeamID;

	TeamActor->RegisterPlayer(P, Index);
	OutPlayers.Add(P);
}

void ADefaultGameMode::BuildBaseFormation()
{
	BaseFormation_Local.Empty(11);

	// GK
	BaseFormation_Local.Add(FVector(-HalfLength + 500.f, 0.f, 0.f)); // 0

	// DEF
	const float DefX = -HalfLength * 0.5f;
	BaseFormation_Local.Add(FVector(DefX, -1400.f, 0.f)); // 1
	BaseFormation_Local.Add(FVector(DefX, -400.f, 0.f)); // 2
	BaseFormation_Local.Add(FVector(DefX, 400.f, 0.f)); // 3
	BaseFormation_Local.Add(FVector(DefX, 1400.f, 0.f)); // 4

	// MID
	const float MidX = 0.f;
	BaseFormation_Local.Add(FVector(MidX, -1600.f, 0.f)); // 5
	BaseFormation_Local.Add(FVector(MidX, -500.f, 0.f)); // 6
	BaseFormation_Local.Add(FVector(MidX, 500.f, 0.f)); // 7
	BaseFormation_Local.Add(FVector(MidX, 1600.f, 0.f)); // 8

	// FWD
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
