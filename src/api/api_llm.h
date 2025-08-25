/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include "../utils/msg.h"
#include <Arduino.h>

namespace m5_module_llm {

struct ApiLlmSetupConfig_t {
    String prompt;
    String model              = "qwen2.5-0.5B-prefill-20e";
    String response_format    = "llm.utf-8.stream";
    std::vector<String> input = {"llm.utf-8.stream"};
    bool enoutput             = true;
    bool enkws                = true;
    int max_token_len         = 127;
    // int max_token_len      = 512;

    std::map<String, String> extra_params;

    template <typename T>
    void setParam(const String& key, T value)
    {
        extra_params[key] = String(value);
    }

    void setParam(const String& key, bool value)
    {
        extra_params[key] = value ? "bool_true" : "bool_false";
    }

    void setParam(const String& key, float value)
    {
        char buffer[16];
        dtostrf(value, 1, 6, buffer);
        extra_params[key] = String(buffer);
    }
};

class ApiLlm {
public:
    void init(ModuleMsg* moduleMsg);

    /**
     * @brief Setup module LLM, return LLM work_id
     *
     * @param config
     * @param request_id
     * @return String
     */
    String setup(ApiLlmSetupConfig_t config = ApiLlmSetupConfig_t(), String request_id = "llm_setup");

    /**
     * @brief Exit module LLM, return LLM work_id
     *
     * @param work_id
     * @param request_id
     * @return String
     */
    String exit(String work_id, String request_id = "llm_exit");

    /**
     * @brief Inference input data by module LLM
     *
     * @param work_id
     * @param input
     * @param request_id
     * @return int
     */
    int inference(String work_id, String input, String request_id = "llm_inference");

    /**
     * @brief Inference input data by module LLM, and wait inference result
     *
     * @param work_id
     * @param input
     * @param onResult On inference result callback
     * @param timeout
     * @param request_id
     * @return int
     */
    int inferenceAndWaitResult(String work_id, String input, std::function<void(String&)> onResult,
                               uint32_t timeout = 5000, String request_id = "llm_inference");

private:
    ModuleMsg* _module_msg = nullptr;
};

}  // namespace m5_module_llm
