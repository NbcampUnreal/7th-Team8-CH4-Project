#include "HeistLoadingScreenWidget.h"

#include "Components/ProgressBar.h"

void UHeistLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CurrentProgress = 0.0f;
	TargetProgress = 0.0f;
	CurrentFillSpeed = 0.0f;
	bFillCompleted = false;

	SetProgressBarPercent(0.0f);
}

void UHeistLoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bFillCompleted) return;
	if (FMath::IsNearlyEqual(CurrentProgress, TargetProgress)) return;

	CurrentProgress = FMath::FInterpConstantTo(CurrentProgress, TargetProgress, InDeltaTime, CurrentFillSpeed);
	SetProgressBarPercent(CurrentProgress);

	if (FMath::IsNearlyEqual(CurrentProgress, 1.0f))
	{
		bFillCompleted = true;
		OnFillComplete.Broadcast();
	}
}

void UHeistLoadingScreenWidget::OnLoadingStarted(bool bIsSeamless)
{
	OnLoadingStarted_Implementation(bIsSeamless);
}

void UHeistLoadingScreenWidget::OnLoadingStarted_Implementation(bool bIsSeamless)
{
	CurrentProgress = 0.0f;
	bFillCompleted = false;

	if (bIsSeamless)
	{
		TargetProgress = SeamlessIdleMaxProgress;
		CurrentFillSpeed = SeamlessFillSpeed;
	}
	else
	{
		TargetProgress = 0.0f;
		CurrentFillSpeed = 0.0f;
	}

	SetProgressBarPercent(0.0f);
}

void UHeistLoadingScreenWidget::StartFillAnimation()
{
	StartFillAnimation_Implementation();
}

void UHeistLoadingScreenWidget::StartFillAnimation_Implementation()
{
	TargetProgress = 1.0f;
	CurrentFillSpeed = CompletionFillSpeed;
}

void UHeistLoadingScreenWidget::SetProgressBarPercent(float Percent)
{
	if (!IsValid(ProgressBar)) return;

	ProgressBar->SetPercent(Percent);
}
