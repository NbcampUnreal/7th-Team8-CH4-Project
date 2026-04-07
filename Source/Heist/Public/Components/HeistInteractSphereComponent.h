
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interaction/HeistInteractable.h"
#include "HeistInteractSphereComponent.generated.h"

class USphereComponent;
// C++에서 바로 바인드하도록 지원해주는 델레게이트 - RetVal - Return Value를 의미한다, 반환값이 있다는 것
// Dynamic으로 못쓴다!! - 델레게이트 BP 바인딩 안됨 - C++ 전용
DECLARE_DELEGATE_RetVal_OneParam(bool, FCanInteractDelegate, ACharacter* /*Interactor*/);
DECLARE_DELEGATE_RetVal_OneParam(FGameplayTag, FGetAbilityTagDelegate, ACharacter* /*Interactor*/)

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class HEIST_API UHeistInteractSphereComponent : public UActorComponent, public IHeistInteractable
{
	GENERATED_BODY()

public:
	UHeistInteractSphereComponent();
	
	// DT_HeistInteractData -> Sphere의 Size 결정합니다.
	// TableRowName - DT에 등록된 이름을 똑같이 지정 필요
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Interaction")
	FName DataTableRowName;
	
	// DT_InteractData 행 이름 - 에디터에서만 설정
	UPROPERTY(EditDefaultsOnly, Category = "Heist|Interaction")
	TObjectPtr<UDataTable> InteractDataTable;
	
	// C++ 캐릭터 전용 바인드
	FCanInteractDelegate OnCanInteract;
	FGetAbilityTagDelegate OnGetAbilityTag;
	
protected:
	virtual void BeginPlay() override;
	
	// IHeistInteractable 구현
	virtual bool CanInteract_Implementation(ACharacter* Interactor) const override;
	virtual FGameplayTag GetInteractAbilityTag_Implementation(ACharacter* Interactor) const override;
	virtual float GetInteractRadius_Implementation() const override;
	
private:
	UPROPERTY()
	TObjectPtr<USphereComponent> InteractSphere;
	
	UFUNCTION()
	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	float CachedRadius = 100.f;

};
