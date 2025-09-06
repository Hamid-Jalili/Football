// DefaultGameMode.cpp
#include "DefaultGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"

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

	// Build formation from DT if provided; otherwise use fallback.
	if (FormationTable)
	{
		BuildFormationFromTable();
	}
	else
	{
		BuildBaseFormation();
		PlayersPerTeam = BaseFormation_Local.Num() > 0 ? BaseFormation_Local.Num() : PlayersPerTeam;
	}

	// Pitch origin (keep world origin unless you have a dedicated field actor).
	FieldCentreWS = FVector::ZeroVector;

	SpawnTeams();

	// Spawn ball at centre (grounded)
	if (BallClass && UGameplayStatics::GetActorOfClass(GetWorld(), BallClass) == nullptr)
	{
		const FVector BallLoc = ProjectXYToGround(FieldCentreWS) + FVector(0, 0, 20.f);
		GetWorld()->SpawnActor<ABallsack>(BallClass, BallLoc, FRotator::ZeroRotator);
	}

	// Ensure we actually possess a blue player at start.
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (Team0Players.IsValidIndex(0) && Team0Players[0])
		{
			PC->Possess(Team0Players[0]);
		}

		// (Optional) keep view on any placed camera:
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

	// Clamp XY inside pitch, then project to ground for correct Z.
	const FVector XYWorld = ClampToField(ToWorld(LocalForTeam));
	FVector SpawnLoc = ProjectXYToGround(XYWorld);

	FRotator SpawnRot(0.f, TeamHalfAngle(TeamID), 0.f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, SpawnRot, Params);
	if (!P) return;

	P->TeamID = TeamID;

	// Optional: apply DT role if present
	if (BaseRoles.IsValidIndex(Index))
	{
		P->PlayerRole = BaseRoles[Index];
	}

	// Final safety—raise to sit on the ground even on slopes
	SnapActorToGround(P);

	TeamActor->RegisterPlayer(P, Index);
	OutPlayers.Add(P);
}

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
	TArray<FRowTmp> Rows;
	Rows.Reserve(RowNames.Num());

	for (const FName& RowName : RowNames)
	{
		const FFormationRow* Row = FormationTable->FindRow<FFormationRow>(RowName, TEXT("FormationRead"));
		if (!Row) continue;
		Rows.Add({ Row->Index, Row->LocalPosition, Row->Role });
	}

	Rows.Sort([](const FRowTmp& A, const FRowTmp& B) { return A.Index < B.Index; });

	for (const FRowTmp& R : Rows)
	{
		BaseFormation_Local.Add(R.Pos);
		BaseRoles.Add(R.Role);
	}

	PlayersPerTeam = BaseFormation_Local.Num();

	// Safety
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
