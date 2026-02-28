#ifndef WJH_CHAT_A7B3C9D1E5F6482394AD8E1F2C3B4A56
#define WJH_CHAT_A7B3C9D1E5F6482394AD8E1F2C3B4A56

#include "wjh/chat/types.hpp"

#include <optional>

namespace wjh::chat {

struct TokenUsage
{
    PromptTokens prompt_tokens{};
    CompletionTokens completion_tokens{};
    TotalTokens total_tokens{};
};

struct ChatResponse
{
    AssistantResponse response;
    std::optional<TokenUsage> usage;
};

} // namespace wjh::chat

#endif
