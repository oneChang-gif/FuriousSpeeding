#include "Subsystem/ChatSubsystem.h"

void UChatSubsystem::AddChatMessage(const FString& InSender, const FString& InMessage)
{
	while (ChatLog.Num() >= MaxChatLog)
	{
		ChatLog.RemoveAt(0);
	}

	ChatLog.Emplace(InSender, InMessage);
}
