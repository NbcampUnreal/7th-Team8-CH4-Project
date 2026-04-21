#include "HeistPlayUISubsystem.h"
#include "HeistPlayHUD.h"
#include "Systems/Messaging/HeistMessageSubsystem.h"
#include "Systems/Messaging/HeistMessageTypes.h"
#include "Systems/Messaging/HeistTags_Message.h"
#include "Blueprint/UserWidget.h"

void UHeistPlayUISubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UHeistMessageSubsystem& MS = UHeistMessageSubsystem::Get(this);

	ReadyPhaseHandle = MS.RegisterListener<FHeistBriefingContextReadyMessage>(
		HeistMessageTags::Message_Briefing_ContextReady,
		[this](FGameplayTag Channel, const FHeistBriefingContextReadyMessage& Msg)
		{
			ShowPlayHUD();
		});
	//TODO_CSH 게임 종류 태그 생성시 변경 필요할 수 있음
	EndHandle = MS.RegisterListener<FHeistBriefingEndMessage>(
		HeistMessageTags::Message_Phase_Changed,
		[this](FGameplayTag Channel, const FHeistBriefingEndMessage& Msg)
		{
			HidePlayHUD();
		});
}

void UHeistPlayUISubsystem::Deinitialize()
{
	HidePlayHUD();
	ReadyPhaseHandle.Unregister();
	EndHandle.Unregister();	
	Super::Deinitialize();
}

void UHeistPlayUISubsystem::ShowPlayHUD()
{
	if (IsValid(PlayHUDInstance))
	{
		PlayHUDInstance->SetVisibility(ESlateVisibility::Visible);
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	PlayHUDInstance = CreateWidget<UHeistPlayHUD>(World, UHeistPlayHUD::StaticClass());
	if (!IsValid(PlayHUDInstance)) return;

	PlayHUDInstance->NativeConstruct(); // 커스텀 초기화 필요 시
	PlayHUDInstance->AddToViewport();
}

void UHeistPlayUISubsystem::HidePlayHUD()
{
	if (!IsValid(PlayHUDInstance)) return;

	PlayHUDInstance->RemoveFromParent();
	PlayHUDInstance = nullptr;
}
