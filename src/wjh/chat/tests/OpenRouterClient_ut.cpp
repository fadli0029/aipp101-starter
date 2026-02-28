// ----------------------------------------------------------------------
// Copyright 2025 Jody Hagins
// Distributed under the MIT Software License
// See accompanying file LICENSE or copy at
// https://opensource.org/licenses/MIT
// ----------------------------------------------------------------------
#define DOCTEST_CONFIG_ASSERTS_RETURN_VALUES
#include "wjh/chat/client/OpenRouterClient.hpp"

#include "wjh/chat/conversation/Conversation.hpp"

#include "testing/doctest.hpp"

namespace {
using namespace wjh::chat;
using namespace wjh::chat::client;
using namespace wjh::chat::conversation;

OpenRouterClientConfig
makeTestConfig()
{
    return OpenRouterClientConfig{
        .api_key = ApiKey("test-api-key"),
        .model = ModelId("openai/gpt-4"),
        .max_tokens = MaxTokens(4096u),
        .system_prompt = SystemPrompt{"Test system prompt"},
        .temperature = std::nullopt};
}

TEST_SUITE("OpenRouterClient")
{
    TEST_CASE("Client configuration")
    {
        SUBCASE("Basic configuration") {
            auto config = makeTestConfig();
            OpenRouterClient client(std::move(config));

            CHECK(client.model() == ModelId("openai/gpt-4"));
        }

        SUBCASE("Without system prompt") {
            OpenRouterClientConfig config{
                .api_key = ApiKey("test-key"),
                .model = ModelId("meta-llama/llama-3-70b-instruct"),
                .max_tokens = MaxTokens(2048u),
                .system_prompt = std::nullopt,
                .temperature = std::nullopt};

            OpenRouterClient client(std::move(config));
            CHECK(client.model() == ModelId("meta-llama/llama-3-70b-instruct"));
        }

        SUBCASE("With temperature") {
            OpenRouterClientConfig config{
                .api_key = ApiKey("test-key"),
                .model = ModelId("openai/gpt-4"),
                .max_tokens = MaxTokens(4096u),
                .system_prompt = std::nullopt,
                .temperature = Temperature{0.7f}};

            OpenRouterClient client(std::move(config));
            CHECK(client.model() == ModelId("openai/gpt-4"));
        }
    }

    TEST_CASE("Message conversion scenarios")
    {
        SUBCASE("Empty conversation") {
            OpenRouterClient client(makeTestConfig());
            Conversation conversation;
            CHECK(conversation.empty());
        }

        SUBCASE("Simple text message") {
            OpenRouterClient client(makeTestConfig());
            Conversation conversation;
            conversation.add_message(UserInput{"Hello, world!"});

            CHECK(conversation.size() == 1);
        }

        SUBCASE("Multi-turn conversation") {
            Conversation conversation;
            conversation.add_message(UserInput{"Hello"});
            conversation.add_message(AssistantResponse{"Hi there!"});
            conversation.add_message(UserInput{"How are you?"});

            CHECK(conversation.size() == 3);
        }
    }

    TEST_CASE("build_request includes tools array")
    {
        OpenRouterClient client(makeTestConfig());
        Conversation conversation;
        conversation.add_message(UserInput{"Hello"});

        auto request = client.build_request(conversation);

        REQUIRE(request.contains("tools"));
        auto const & tools = request["tools"];
        REQUIRE(tools.is_array());
        REQUIRE(tools.size() == 1);

        auto const & tool = tools[0];
        CHECK(tool["type"] == "function");
        CHECK(tool["function"]["name"] == "bash");
        CHECK(tool["function"]["parameters"]["type"] == "object");
        CHECK(tool["function"]["parameters"]["properties"].contains("command"));
        CHECK(tool["function"]["parameters"]["required"][0] == "command");
    }

    TEST_CASE("parse_response formats tool calls")
    {
        OpenRouterClient client(makeTestConfig());

        SUBCASE("Single tool call") {
            auto json = nlohmann::json::parse(R"({
                "choices": [{
                    "message": {
                        "role": "assistant",
                        "content": null,
                        "tool_calls": [{
                            "id": "call_abc123",
                            "type": "function",
                            "function": {
                                "name": "bash",
                                "arguments": "{\"command\":\"ls src/\"}"
                            }
                        }]
                    }
                }],
                "usage": {
                    "prompt_tokens": 10,
                    "completion_tokens": 5,
                    "total_tokens": 15
                }
            })");

            auto result = client.parse_response(json);
            REQUIRE(result.has_value());
            auto const & text = atlas::undress(result->response);
            CHECK(text == "[Tool call] bash: {\"command\":\"ls src/\"}\n");
        }

        SUBCASE("Multiple tool calls") {
            auto json = nlohmann::json::parse(R"({
                "choices": [{
                    "message": {
                        "role": "assistant",
                        "content": null,
                        "tool_calls": [
                            {
                                "id": "call_1",
                                "type": "function",
                                "function": {
                                    "name": "bash",
                                    "arguments": "{\"command\":\"ls\"}"
                                }
                            },
                            {
                                "id": "call_2",
                                "type": "function",
                                "function": {
                                    "name": "bash",
                                    "arguments": "{\"command\":\"pwd\"}"
                                }
                            }
                        ]
                    }
                }]
            })");

            auto result = client.parse_response(json);
            REQUIRE(result.has_value());
            auto const & text = atlas::undress(result->response);
            CHECK(text == "[Tool call] bash: {\"command\":\"ls\"}\n"
                          "[Tool call] bash: {\"command\":\"pwd\"}\n");
        }

        SUBCASE("Empty tool_calls falls through to content check") {
            auto json = nlohmann::json::parse(R"({
                "choices": [{
                    "message": {
                        "role": "assistant",
                        "content": null,
                        "tool_calls": []
                    }
                }]
            })");

            auto result = client.parse_response(json);
            CHECK(not result.has_value());
        }
    }
}

} // anonymous namespace
