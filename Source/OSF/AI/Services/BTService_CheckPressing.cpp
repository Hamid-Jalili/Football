//#include "BTService_CheckPressing.h"
//#include "AIController.h"
//#include "BehaviorTree/BlackboardComponent.h"
//#include "Kismet/GameplayStatics.h"
//
//UBTService_CheckPressing::UBTService_CheckPressing()
//{
//	bNotifyBecomeRelevant = false;
//	bNotifyCeaseRelevant = false;
//	Interval = 0.2f;
//	RandomDeviation = 0.02f;
//}
//
//static AActor* FindBall(UWorld* W)
//{
//	TArray<AActor*> Found;
//	UGameplayStatics::GetAllActorsWithTag(W, FName("Ball"), Found);
//	return Found.Num() ? Found[0] : nullptr;
//}
//
//void UBTService_CheckPressing::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
//{
//	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
//
//	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
//	AAIController* AIC = OwnerComp.GetAIOwner();
//	APawn* Pawn = AIC ? AIC->GetPawn() : nullptr;
//	UWorld* W = Pawn ? Pawn->GetWorld() : nullptr;
//	if (!BB || !Pawn || !W) return;
//
//	const AActor* Ball = FindBall(W);
//	const FVector MyLoc = Pawn->GetActorLocation();
//
//	bool bPress = false;
//	FVector Target = MyLoc;
//
//	if (Ball)
//	{
//		const FVector BallLoc = Ball->GetActorLocation();
//		Target = BallLoc;
//
//		// Toy rule: press if ball is within 2500 cm (25m)
//		bPress = (FVector::DistSquared(MyLoc, BallLoc) <= 2500.f * 2500.f);
//	}
//
//	BB->SetValueAsBool(Key_ShouldBePressing, bPress);
//	BB->SetValueAsVector(Key_TargetLocation, Target);
//}
