#include "DefaultGameMode.h"
#include "Ballsack.h"                      // <-- MUST be FIRST (Ball class is referenced in BeginPlay)
#include "OSF.h"
#include "TeamGameState.h"
#include "FootballerController.h"
#include "FootballTeam.h"
#include "Footballer.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"

// --------------------------------------------------------------------------------------
// Local helpers
// --------------------------------------------------------------------------------------

/**
 * Find a PlayerStart for a given side.
 * Preferred: PlayerStartTag == "Home"/"Away"
 * Fallback:  left-most X for Home, right-most X for Away
 */
static APlayerStart* FindPlayerStartForSide(UWorld* World, bool bHomeSide)
{
    if (!World) return nullptr;

    APlayerStart* Tagged = nullptr;
    float BestX = bHomeSide ? +FLT_MAX : -FLT_MAX; // home -> smallest X, away -> largest X
    APlayerStart* Fallback = nullptr;

    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        APlayerStart* PS = *It;
#if ENGINE_MAJOR_VERSION >= 5
        // 5.x has PlayerStartTag (Name). If not present, Tags array may be used by user.
        if (PS->PlayerStartTag == (bHomeSide ? FName("Home") : FName("Away")))
        {
            Tagged = PS;
            break;
        }
#endif
        const float X = PS->GetActorLocation().X;
        const bool bBetter = bHomeSide ? (X < BestX) : (X > BestX);
        if (bBetter)
        {
            BestX = X;
            Fallback = PS;
        }
    }

    return (Tagged != nullptr) ? Tagged : Fallback;
}

// --------------------------------------------------------------------------------------
// ADefaultGameMode
// --------------------------------------------------------------------------------------

static UClass* BPBallClass = nullptr;

ADefaultGameMode::ADefaultGameMode()
{
    static ConstructorHelpers::FObjectFinder<UClass> BPBallFinder(
        TEXT("Blueprint'/Game/Blueprints/BPBall.BPBall_C'")
    );
    BPBallClass = BPBallFinder.Object;
}

void ADefaultGameMode::BeginPlay()
{
    Super::BeginPlay();

    NextControllerIsHome = true;

    if (HasAuthority())
    {
        // Example for local split-screen: spawn extra local players if desired
        // UGameplayStatics::CreatePlayer(GetWorld(), /*ControllerId*/1, /*bSpawnPawn*/false);
    }
}

void ADefaultGameMode::StartPlay()
{
    // Super::StartPlay calls StartMatch; keep logic in StartMatch.
    Super::StartPlay();
}

void ADefaultGameMode::StartMatch()
{
    IsSetup = true;

    if (ABallsack::GetWorldBall(GetWorld()) == nullptr)
    {
        // No ball in the level = nothing to do.
        Super::StartMatch();
        return;
    }

    if (HasAuthority())
    {
        ATeamGameState* State = GetWorld()->GetGameState<ATeamGameState>();

        // Create teams (and ensure ball exists)
        State->HomeTeam = GetWorld()->SpawnActor<AFootballTeam>();
        State->AwayTeam = GetWorld()->SpawnActor<AFootballTeam>();

        State->Ball = ABallsack::GetWorldBall(GetWorld()); // or spawn using BPBallClass if desired

        State->LoadSampleState();

        check(State && State->HomeTeam && State->AwayTeam);

        // Assign each connected PlayerController to a footballer
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            AFootballerController* Controller = Cast<AFootballerController>(*It);
            if (Controller)
            {
                PlaceController(Controller);
            }
        }
    }

    Super::StartMatch();
}

void ADefaultGameMode::PlaceController(AFootballerController* Controller)
{
    if (!IsSetup || !Controller) return;
    check(HasAuthority());

    ATeamGameState* State = GetWorld()->GetGameState<ATeamGameState>();
    check(State);

    // Alternate controllers between home and away
    const bool bGiveHome = NextControllerIsHome;
    NextControllerIsHome = !NextControllerIsHome;

    AFootballTeam* Team = bGiveHome ? State->HomeTeam : State->AwayTeam;
    check(Team && Team->Footballers.Num() > 0);

    // Ensure the controller has no existing pawn
    if (APawn* Pawn = Controller->GetPawn())
    {
        Controller->UnPossess();
        GetWorld()->DestroyActor(Pawn);
    }

    // Pick the first footballer that is not controlled by a player yet
    AFootballer* Chosen = nullptr;
    for (int32 Idx = 0; Idx < Team->Footballers.Num(); ++Idx)
    {
        AFootballer* F = Team->Footballers[Idx];
        if (F && !F->IsControlledByPlayer())
        {
            Chosen = F;
            break;
        }
    }

    if (!Chosen)
    {
        // Fallback: if all are taken, just pick the first
        Chosen = Team->Footballers[0];
    }

    // Put the FIRST controlled footballer for each side near a PlayerStart,
    // so Home/ Away do NOT spawn overlapped at centre.
    if (APlayerStart* PS = FindPlayerStartForSide(GetWorld(), bGiveHome))
    {
        FVector L = PS->GetActorLocation();
        // Small offset to ensure not exactly on the PlayerStart capsule if two sides share close spawns
        L += FVector(bGiveHome ? +150.f : -150.f, 0.f, 0.f);
        Chosen->SetActorLocation(L);
        Chosen->SetActorRotation(PS->GetActorRotation());
    }
    else
    {
        // Fallback if no PlayerStart actors exist in the map
        FVector DefaultLeft(-3000.f, 0.f, 100.f);
        FVector DefaultRight(+3000.f, 0.f, 100.f);
        Chosen->SetActorLocation(bGiveHome ? DefaultLeft : DefaultRight);
    }

    // Possess
    Controller->SwitchToFootballer(Chosen);
}
