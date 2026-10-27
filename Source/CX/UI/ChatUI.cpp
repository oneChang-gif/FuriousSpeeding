#include "ChatUI.h"
#include "CXPlayerController.h"
#include "Subsystem/ChatSubsystem.h"

void UChatUI::NativeConstruct()
{
	Super::NativeConstruct();

	if (EditableTextBox_ChatInput && !EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UChatUI::OnChatInputTextCommitted))
	{
		EditableTextBox_ChatInput->OnTextCommitted.AddDynamic(this, &UChatUI::OnChatInputTextCommitted);
	}
}

void UChatUI::NativeDestruct()
{
	if (EditableTextBox_ChatInput && EditableTextBox_ChatInput->OnTextCommitted.IsAlreadyBound(this, &UChatUI::OnChatInputTextCommitted))
	{
		EditableTextBox_ChatInput->OnTextCommitted.RemoveDynamic(this, &UChatUI::OnChatInputTextCommitted);
	}

	Super::NativeDestruct();
}

void UChatUI::OnNewChatMessage(const FString& InSender, const FString& InMessage)
{
	if (!MultilineEditableTextBox_ChatLog) return;
	FString NewMessage = InSender + TEXT(": ") + InMessage;

	FString OldMessage = MultilineEditableTextBox_ChatLog->GetText().ToString();
	if (!OldMessage.IsEmpty()) OldMessage += '\n';
	FText Message = FText::FromString(OldMessage + NewMessage);

	MultilineEditableTextBox_ChatLog->SetText(Message);
	if (ChatScrollBox) ChatScrollBox->ScrollToEnd();
}

void UChatUI::SetChatLog(const TArray<FChatData>& ChatLog)
{
	if (!MultilineEditableTextBox_ChatLog) return;
	FString Log;
	for (const FChatData& Entry : ChatLog)
	{
		if (!Log.IsEmpty()) Log += TEXT("\n");
		Log += Entry.Sender + TEXT(": ") + Entry.Message;
	}
	MultilineEditableTextBox_ChatLog->SetText(FText::FromString(Log));
	if (ChatScrollBox) ChatScrollBox->ScrollToEnd();
}

void UChatUI::OnChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if(CommitMethod == ETextCommit::OnEnter)
	{
		if (Text.IsEmpty() == false)
		{
			FString ChatMessage = Text.ToString();
			EditableTextBox_ChatInput->SetText(FText::GetEmpty());
			if (ACXPlayerController* PC = GetOwningPlayer<ACXPlayerController>())
			{
				PC->CTSChatMessage(ChatMessage);
			}
		}
	}
}
