#include "CXUI.h"
#include "UI/ChatUI.h"
#include "Subsystem/ChatSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "GameState/CXGameState.h"

void UCXUI::NativeConstruct()
{
	Super::NativeConstruct();

	UpdateGameResult(false, false);

	if (IsValid(MatchStatusText))
	{
		MatchStatusText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (IsValid(GameStateText) == false)
	{
		GameStateText = CreateMatchText(TEXT("GameStateText"), 55.0f);
	}

	if (IsValid(RemainingTimeText) == false)
	{
		RemainingTimeText = CreateMatchText(TEXT("RemainingTimeText"), 90.0f);
	}

	if (IsValid(GameStateText))
	{
		GameStateText->SetText(NSLOCTEXT("CX", "WaitingForMatchState", "경기 정보 수신 중"));
	}

	if (IsValid(RemainingTimeText))
	{
		RemainingTimeText->SetText(FText::GetEmpty());
	}

	if (!PlayerStateInfoText && WidgetTree)
	{
		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			PlayerStateInfoText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PlayerStateInfoText"));
			PlayerStateInfoText->SetText(FText::FromString(TEXT("Coins: 0, BonusMaxSpeed: 0.00")));
			FSlateFontInfo Font = PlayerStateInfoText->GetFont();
			Font.Size = 16;
			PlayerStateInfoText->SetFont(Font);
			PlayerStateInfoText->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* InfoSlot = Canvas->AddChildToCanvas(PlayerStateInfoText);
			InfoSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			InfoSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			InfoSlot->SetPosition(FVector2D(0.0f, 20.0f));
			InfoSlot->SetAutoSize(true);
		}
	}
}

void UCXUI::UpdatePlayerStateInfo(const FString& Info)
{
	if (PlayerStateInfoText) PlayerStateInfoText->SetText(FText::FromString(Info));
}

void UCXUI::SetChatLog(const TArray<FChatData>& ChatLog)
{
	if (UI_Chat) UI_Chat->SetChatLog(ChatLog);
}

void UCXUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	if (UI_Chat) UI_Chat->OnNewChatMessage(InSender, InMessage);
}

void UCXUI::UpdateSpeed(float NewSpeed)
{
	// format the speed to KPH or MPH
	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	// call the Blueprint handler
	OnSpeedUpdate(FormattedSpeed);
}

void UCXUI::UpdateGear(int32 NewGear)
{
	// call the Blueprint handler
	OnGearUpdate(NewGear);
}

void UCXUI::UpdateGameResult(bool bFinished, bool bWinner)
{
	if (IsValid(ResultText) == false)
	{
		return;
	}

	if (bFinished == false)
	{
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FText Result = NSLOCTEXT("CX", "Defeat", "패배");

	if (bWinner == true)
	{
		Result = NSLOCTEXT("CX", "Victory", "승리!");
	}

	if (ResultText->GetText().EqualTo(Result) == false)
	{
		ResultText->SetText(Result);
	}

	ResultText->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UCXUI::UpdateMatchStatus(EGameState GameState, int32 RemainingTime)
{
	FText StateText = NSLOCTEXT("CX", "Waiting", "대기");
	if (GameState == EGameState::Playing)
	{
		StateText = NSLOCTEXT("CX", "Playing", "진행");
	}

	if (GameState == EGameState::Finished)
	{
		StateText = NSLOCTEXT("CX", "Finished", "종료");
	}

	if (IsValid(GameStateText))
	{
		if (GameStateText->GetText().EqualTo(StateText) == false)
		{
			GameStateText->SetText(StateText);
		}
		GameStateText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const FText TimeText = FText::Format(
		NSLOCTEXT("CX", "RemainingTime", "남은 시간: {0}초"),
		FText::AsNumber(FMath::Max(0, RemainingTime)));

	if (IsValid(RemainingTimeText))
	{
		if (RemainingTimeText->GetText().EqualTo(TimeText) == false)
		{
			RemainingTimeText->SetText(TimeText);
		}
		RemainingTimeText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

UTextBlock* UCXUI::CreateMatchText(FName WidgetName, float Top)
{
	if (WidgetTree == nullptr)
	{
		return nullptr;
	}

	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	UOverlay* Overlay = Cast<UOverlay>(WidgetTree->RootWidget);
	if (Canvas == nullptr && Overlay == nullptr)
	{
		return nullptr;
	}

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), WidgetName);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 24;
	Text->SetFont(Font);
	Text->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (Canvas)
	{
		UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(Text);
		TextSlot->SetAnchors(FAnchors(0.5f, 0.0f));
		TextSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		TextSlot->SetPosition(FVector2D(0.0f, Top));
		TextSlot->SetAutoSize(true);
	}

	if (Overlay)
	{
		UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(Text);
		TextSlot->SetHorizontalAlignment(HAlign_Center);
		TextSlot->SetVerticalAlignment(VAlign_Top);
		TextSlot->SetPadding(FMargin(0.0f, Top, 0.0f, 0.0f));
	}

	return Text;
}
