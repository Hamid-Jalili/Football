//#pragma once
//
//#include "CoreMinimal.h"
//#include "BehaviorTree/BTService.h"
//#include "BTService_CheckPressing.generated.h"
//
///**
// * Writes ShouldBePressing (bool) and TargetLocation (Vector) each tick.
// * Very simple heuristic you can refine later.
// */
//UCLASS()
//class OSF_API UBTService_CheckPressing : public UBTService
//{
//	GENERATED_BODY()
//
//public:
//	UBTService_CheckPressing();
//
//protected:
//	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
//
//	/** Blackboard key names (match your blackboard asset) */
//	UPROPERTY(EditAnywhere, Category = "Blackboard")
//	FName Key_ShouldBePressing = FName("ShouldBePressing");
//
//	UPROPERTY(EditAnywhere, Category = "Blackboard")
//	FName Key_TargetLocation = FName("TargetLocation");
//};
