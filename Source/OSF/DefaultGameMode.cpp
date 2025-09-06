#include "DefaultGameMode.h"

#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Camera/CameraActor.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Footballer.h"
#include "FootballTeam.h"
#include "Ballsack.h"
#include "FormationRow.h"

ADefaultGameMode::ADefaultGameMode()
{
    PrimaryActorTick.bCanEverTick = false; // timer-driven
    Team0Players.Reserve(11);
    Team1Players.Reserve(11);
}

void ADefaultGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (FormationTable) BuildFormationFromTable();
    else
    {
        BuildBaseFormation();
        PlayersPerTeam = BaseFormation_Local.Num() > 0 ? BaseFormation_Local.Num() : PlayersPerTeam;
    }

    FieldCentreWS = FVector::ZeroVector;
    SpawnTeams();

    // Ball
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
        if (Team0Players.IsValidIndex(0) && Team0Players[0]) PC->Possess(Team0Players[0]);
        if (ACameraActor* LevelCam = Cast<ACameraActor>(UGameplayStatics::GetActorOfClass(GetWorld(), ACameraActor::StaticClass())))
        {
            PC->SetViewTargetWithBlend(LevelCam, 0.f);
        }
    }

    GetWorldTimerManager().SetTimer(ThinkTimer, this, &ADefaultGameMode::Think, 0.1f, true); // 10 Hz
}

// ---------------- Possession ----------------
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
    if (!TeamClass || !FootballerClass) return;

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
    const FRotator SpawnRot(0.f, TeamHalfAngle(TeamID), 0.f);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AFootballer* P = GetWorld()->SpawnActor<AFootballer>(FootballerClass, SpawnLoc, SpawnRot, Params);
    if (!P) return;

    P->TeamID = TeamID;

    // Optional base role
    if (BaseRoles.IsValidIndex(Index))
    {
        P->PlayerRole = static_cast<EFootballRole>(BaseRoles[Index]);
    }

    SnapActorToGround(P);

    if (P->GetController() == nullptr) P->SpawnDefaultController();

    if (ACharacter* C = Cast<ACharacter>(P))
    {
        if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
        {
            Move->bOrientRotationToMovement = true;
            Move->bUseControllerDesiredRotation = false;
            Move->RotationRate = FRotator(0.f, 720.f, 0.f);
        }
    }

    TeamActor->RegisterPlayer(P, Index);
    OutPlayers.Add(P); // keep spawn order == formation slot
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

    struct FRowTmp { int32 Index; FVector Pos; uint8 Role; };
    TArray<FRowTmp> Rows; Rows.Reserve(RowNames.Num());

    for (const FName& RowName : RowNames)
    {
        const FFormationRow* Row = FormationTable->FindRow<FFormationRow>(RowName, TEXT("FormationRead"));
        if (!Row) continue;
        Rows.Add({ Row->Index, Row->LocalPosition, static_cast<uint8>(Row->Role) });
    }

    Rows.Sort([](const FRowTmp& A, const FRowTmp& B) { return A.Index < B.Index; });

    for (const FRowTmp& R : Rows)
    {
        BaseFormation_Local.Add(R.Pos);
        BaseRoles.Add(R.Role);
    }

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

    // GK (index 0)
    BaseFormation_Local.Add(FVector(-HalfLength + 500.f, 0.f, 0.f)); // 0

    // DEF
    const float DefX = -HalfLength * 0.5f;
    BaseFormation_Local.Add(FVector(DefX, -1400.f, 0.f)); // 1
    BaseFormation_Local.Add(FVector(DefX, -400.f, 0.f)); // 2
    BaseFormation_Local.Add(FVector(DefX, +400.f, 0.f)); // 3
    BaseFormation_Local.Add(FVector(DefX, +1400.f, 0.f)); // 4

    // MID
    const float MidX = 0.f;
    BaseFormation_Local.Add(FVector(MidX, -1600.f, 0.f)); // 5
    BaseFormation_Local.Add(FVector(MidX, -500.f, 0.f)); // 6
    BaseFormation_Local.Add(FVector(MidX, 500.f, 0.f)); // 7
    BaseFormation_Local.Add(FVector(MidX, +1600.f, 0.f)); // 8

    // FWD
    const float FwdX = HalfLength * 0.35f;
    BaseFormation_Local.Add(FVector(FwdX, -800.f, 0.f)); // 9
    BaseFormation_Local.Add(FVector(FwdX, +800.f, 0.f)); // 10
}

FVector ADefaultGameMode::FormationLocal(int32 Index) const
{
    return BaseFormation_Local.IsValidIndex(Index) ? BaseFormation_Local[Index] : FVector::ZeroVector;
}
FVector ADefaultGameMode::ToWorld(const FVector& Local) const { return FieldCentreWS + Local; }

FVector ADefaultGameMode::ClampToField(const FVector& P) const
{
    FVector Out = P;
    Out.X = FMath::Clamp(Out.X, FieldCentreWS.X - HalfLength, FieldCentreWS.X + HalfLength);
    Out.Y = FMath::Clamp(Out.Y, FieldCentreWS.Y - HalfWidth, FieldCentreWS.Y + HalfWidth);
    return Out;
}
float ADefaultGameMode::TeamHalfAngle(int32 TeamID) const { return (TeamID == 0) ? 0.f : 180.f; }

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

// ---------------- Brain ----------------
void ADefaultGameMode::Think()
{
    int32 AttackingTeam = PossessingTeamID;

    const AActor* BallActor = Ball ? static_cast<AActor*>(Ball)
        : UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass());
    const FVector BallLoc = BallActor ? BallActor->GetActorLocation() : FieldCentreWS;

    if (AttackingTeam < 0)
    {
        auto MinDistToBall = [&](const TArray<AFootballer*>& Team)->float
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

static AFootballer* ClosestTo(const TArray<AFootballer*>& Arr, const FVector& Pt, const TSet<AFootballer*>& Exclude = {})
{
    AFootballer* Best = nullptr; float BestD = TNumericLimits<float>::Max();
    for (AFootballer* P : Arr)
    {
        if (!IsValid(P) || Exclude.Contains(P)) continue;
        const float D = FVector::DistSquared(P->GetActorLocation(), Pt);
        if (D < BestD) { BestD = D; Best = P; }
    }
    return Best;
}

void ADefaultGameMode::ComputeKeeperTarget(
    int32 TeamID, const FVector& BallLoc,
    FVector& OutGoal, FVector& OutKeeperHome, FVector& OutBoxMin, FVector& OutBoxMax) const
{
    OutGoal = OwnGoalLocation(TeamID);

    const float Dir = (TeamID == 0) ? +1.f : -1.f;
    const float BoxX0 = OutGoal.X + Dir * PenBoxDepth;
    OutBoxMin = FVector(FMath::Min(OutGoal.X, BoxX0), FieldCentreWS.Y - PenBoxHalfWidth, OutGoal.Z);
    OutBoxMax = FVector(FMath::Max(OutGoal.X, BoxX0), FieldCentreWS.Y + PenBoxHalfWidth, OutGoal.Z);

    const FVector ToBall = (BallLoc - OutGoal).GetSafeNormal2D();
    FVector Candidate = OutGoal + ToBall * KeeperDepth;
    Candidate.X = FMath::Clamp(Candidate.X, OutBoxMin.X, OutBoxMax.X);
    Candidate.Y = FMath::Clamp(Candidate.Y, OutBoxMin.Y, OutBoxMax.Y);

    OutKeeperHome = ProjectXYToGround(ClampToField(Candidate));
}

void ADefaultGameMode::DriveTeamAI(const TArray<AFootballer*>& Team, int32 TeamID, bool bAttacking)
{
    AActor* BallActor = Ball ? static_cast<AActor*>(Ball)
        : UGameplayStatics::GetActorOfClass(GetWorld(), ABallsack::StaticClass());
    const FVector BallLoc = BallActor ? BallActor->GetActorLocation() : FieldCentreWS;

    const float Dir = (TeamID == 0) ? +1.f : -1.f;
    const FVector OwnGoal = OwnGoalLocation(TeamID);

    // Two closest to ball
    TSet<AFootballer*> Used;
    AFootballer* First = ClosestTo(Team, BallLoc);
    if (First)  Used.Add(First);
    AFootballer* Second = ClosestTo(Team, BallLoc, Used);
    if (Second) Used.Add(Second);

    // Keeper = slot 0
    AFootballer* Keeper = Team.IsValidIndex(0) ? Team[0] : nullptr;

    // Remaining indices (we keep slot index explicitly)
    TArray<int32> RestIdx;
    RestIdx.Reserve(Team.Num());
    for (int32 i = 0; i < Team.Num(); ++i)
    {
        AFootballer* P = Team[i];
        if (!IsValid(P)) continue;
        if (i == 0) continue;           // GK handled separately
        if (P == First || P == Second) continue;
        RestIdx.Add(i);
    }

    // Move helper: blend tactical with home slot
    auto SetTargetFor = [&](AFootballer* P, int32 SlotIndex, const FVector& Tactical, EPlayRole PlayIntent)
        {
            if (!IsValid(P)) return;

            FVector HomeLocal = FormationLocal(SlotIndex);
            if (TeamID == 1) HomeLocal = FVector(-HomeLocal.X, -HomeLocal.Y, HomeLocal.Z);
            FVector HomeWorld = ProjectXYToGround(ClampToField(ToWorld(HomeLocal)));

            const FVector Target = FMath::Lerp(HomeWorld, Tactical, 1.f - HomeWeight);

            // steering intent for your pawn
            FVector Desired = SeekArriveDirection(P->GetActorLocation(), Target);
            Desired += SeparationVector(P, Team) * SeparationStrength;

            P->SetDesiredMovement(Desired);
            P->SetDesiredSprintStrength((Target - P->GetActorLocation()).Size() > 700.f ? 1.f : 0.f);

            if (AAIController* AIC = Cast<AAIController>(P->GetController()))
            {
                AIC->MoveToLocation(Target, ArriveRadius, true, true, true, false);
                if (BallActor) AIC->SetFocus(BallActor);
            }
            else if (ACharacter* C = Cast<ACharacter>(P))
            {
                C->AddMovementInput(Desired, 1.f);
            }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
            const FColor CCol = (PlayIntent == EPlayRole::Press) ? FColor::Red
                : (PlayIntent == EPlayRole::Support) ? FColor::Yellow
                : (PlayIntent == EPlayRole::Mark) ? FColor::Cyan
                : FColor::Green;
            DrawDebugDirectionalArrow(GetWorld(), P->GetActorLocation(), Target, 40.f, CCol, false, 0.12f, 0, 2.f);
#endif
        };

    // --- Keeper ---
    if (IsValid(Keeper))
    {
        FVector Goal, KeeperHome, BoxMin, BoxMax;
        ComputeKeeperTarget(TeamID, BallLoc, Goal, KeeperHome, BoxMin, BoxMax);

        const float DistToBall = FVector::Dist2D(Keeper->GetActorLocation(), BallLoc);
        FVector GKTarget = KeeperHome;

        // Inside box and close enough → step out toward ball
        if (BallLoc.X >= FMath::Min(BoxMin.X, BoxMax.X) && BallLoc.X <= FMath::Max(BoxMin.X, BoxMax.X) &&
            BallLoc.Y >= BoxMin.Y && BallLoc.Y <= BoxMax.Y &&
            DistToBall <= KeeperChaseRadius)
        {
            GKTarget = ProjectXYToGround(ClampToField(BallLoc));
        }

        const FVector Target = FMath::Lerp(KeeperHome, GKTarget, 0.5f);
        if (AAIController* AIC = Cast<AAIController>(Keeper->GetController()))
        {
            AIC->MoveToLocation(Target, ArriveRadius, true, true, true, false);
            if (BallActor) AIC->SetFocus(BallActor);
        }

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
        DrawDebugBox(GetWorld(), (BoxMin + BoxMax) * 0.5f,
            FVector(FMath::Abs(BoxMax.X - BoxMin.X) * 0.5f, PenBoxHalfWidth, 50.f),
            FColor::White, false, 0.12f, 0, 1.f);
        DrawDebugSphere(GetWorld(), Target, 40.f, 10, FColor::White, false, 0.12f, 0, 2.f);
#endif
    }

    // --- Field players ---
    if (bAttacking)
    {
        const int32 FirstIdx = Team.IndexOfByKey(First);
        const int32 SecondIdx = Team.IndexOfByKey(Second);

        if (IsValid(First) && FirstIdx != INDEX_NONE)
        {
            const FVector T = ClampToField(ProjectXYToGround(BallLoc + FVector(SupportAhead * Dir, +SupportWide, 0.f)));
            SetTargetFor(First, FirstIdx, T, EPlayRole::Support);
        }
        if (IsValid(Second) && SecondIdx != INDEX_NONE)
        {
            const FVector T = ClampToField(ProjectXYToGround(BallLoc + FVector(SupportAhead * Dir, -SupportWide, 0.f)));
            SetTargetFor(Second, SecondIdx, T, EPlayRole::Support);
        }

        for (int32 SlotIdx : RestIdx)
        {
            AFootballer* P = Team[SlotIdx];
            if (!IsValid(P) || (P->GetController() && P->GetController()->IsPlayerController())) continue;

            FVector HomeLocal = FormationLocal(SlotIdx);
            if (TeamID == 1) HomeLocal = FVector(-HomeLocal.X, -HomeLocal.Y, HomeLocal.Z);
            FVector HomeWorld = ToWorld(HomeLocal);
            HomeWorld.X += AdvanceWithBall * 0.5f * Dir;

            const FVector T = ClampToField(ProjectXYToGround(HomeWorld));
            SetTargetFor(P, SlotIdx, T, EPlayRole::HoldLine);
        }
    }
    else
    {
        auto ContainPoint = [&](float side)->FVector
            {
                const FVector ToGoal = (OwnGoal - BallLoc).GetSafeNormal2D();
                const FVector Right = FVector::CrossProduct(ToGoal, FVector::UpVector);
                return BallLoc + ToGoal * 900.f + Right * side * 260.f;
            };

        const int32 FirstIdx = Team.IndexOfByKey(First);
        const int32 SecondIdx = Team.IndexOfByKey(Second);

        if (IsValid(First) && FirstIdx != INDEX_NONE)
            SetTargetFor(First, FirstIdx, ClampToField(ProjectXYToGround(ContainPoint(+1.f))), EPlayRole::Press);
        if (IsValid(Second) && SecondIdx != INDEX_NONE)
            SetTargetFor(Second, SecondIdx, ClampToField(ProjectXYToGround(ContainPoint(-1.f))), EPlayRole::Press);

        const TArray<AFootballer*>& Opp = (TeamID == 0) ? Team1Players : Team0Players;
        TSet<AFootballer*> Claimed;

        for (int32 SlotIdx : RestIdx)
        {
            AFootballer* P = Team[SlotIdx];
            if (!IsValid(P) || (P->GetController() && P->GetController()->IsPlayerController())) continue;

            AFootballer* Att = nullptr; float Best = TNumericLimits<float>::Max();
            for (AFootballer* A : Opp)
            {
                if (!IsValid(A) || Claimed.Contains(A)) continue;
                const float D = FVector::DistSquared(P->GetActorLocation(), A->GetActorLocation());
                if (D < Best) { Best = D; Att = A; }
            }
            if (Att) Claimed.Add(Att);

            FVector MarkPos;
            if (Att)
            {
                const FVector Apos = Att->GetActorLocation();
                const FVector AG = (OwnGoal - Apos).GetSafeNormal2D();
                MarkPos = Apos + AG * 350.f; // goal-side
            }
            else
            {
                FVector Slot = FormationLocal(SlotIdx);
                if (TeamID == 1) Slot = FVector(-Slot.X, -Slot.Y, Slot.Z);
                MarkPos = ToWorld(Slot);
            }

            MarkPos.X -= RetreatWithBall * 0.5f * Dir;
            SetTargetFor(P, SlotIdx, ClampToField(ProjectXYToGround(MarkPos)), EPlayRole::Mark);
        }
    }
}

// ---------- Steering helpers ----------
FVector ADefaultGameMode::SeekArriveDirection(const FVector& From, const FVector& To) const
{
    const FVector ToT = (To - From);
    const float Dist = ToT.Size2D();
    if (Dist < KINDA_SMALL_NUMBER) return FVector::ZeroVector;

    const float Strength = (Dist > ArriveRadius) ? 1.f : (Dist / ArriveRadius);
    return ToT.GetSafeNormal2D() * Strength;
}

FVector ADefaultGameMode::SeparationVector(AFootballer* Self, const TArray<AFootballer*>& SameTeam) const
{
    if (!IsValid(Self)) return FVector::ZeroVector;
    const FVector MyPos = Self->GetActorLocation();
    FVector Accum = FVector::ZeroVector;

    for (AFootballer* P : SameTeam)
    {
        if (!IsValid(P) || P == Self) continue;
        const FVector Delta = MyPos - P->GetActorLocation();
        const float D2 = FMath::Max(Delta.SizeSquared2D(), 1.f);
        if (D2 < SeparationRadius * SeparationRadius)
        {
            Accum += Delta.GetSafeNormal2D() * (SeparationRadius * SeparationRadius / D2);
        }
    }
    return Accum;
}

FVector ADefaultGameMode::OwnGoalLocation(int32 TeamID) const
{
    const float X = FieldCentreWS.X + ((TeamID == 0) ? -HalfLength : +HalfLength);
    return FVector(X, FieldCentreWS.Y, FieldCentreWS.Z);
}
