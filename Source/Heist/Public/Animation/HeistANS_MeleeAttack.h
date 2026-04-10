
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "HeistANS_MeleeAttack.generated.h"

/**
 * 근접 공격 타격 판정 구간 AnimNotifyState
 *
 * NotifyBegin -> NotifyEnd 구간에서 매 틱 지정한 소켓 위치 기준으로 스윕 감지
 * 이전 프레임 소켓 위치 -> 현재 프레임 소켓 위치 사이를 SweepMulti로 검사하여 자연스러운 타격 연출
 * 공격자 컴포넌트에 바인딩된 delegate를 바로 실행함
 */
UCLASS()
class HEIST_API UHeistANS_MeleeAttack : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UHeistANS_MeleeAttack();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float TotalDuration, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

protected:
	// 히트박스를 부착할 소켓 이름 (foot_r 등)
	UPROPERTY(EditAnywhere, Category = "Heist|MeleeAttack")
	FName HitSocket = FName("hand_r");

	// 스윕 구체 크기
	UPROPERTY(EditAnywhere, Category = "Heist|MeleeAttack")
	float HitSphereRadius = 20.f;

	// 감지 채널 설정
	UPROPERTY(EditAnywhere, Category = "Heist|MeleeAttack")
	TEnumAsByte<ECollisionChannel> HitChannel = ECC_Pawn;

private:
	// 중복 히트 방지
	TSet<TWeakObjectPtr<AActor>> AlreadyHitActors;
};
