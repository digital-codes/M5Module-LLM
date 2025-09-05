/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "api_vlm.h"
#include "api_version.h"

using namespace m5_module_llm;

void ApiVlm::init(ModuleMsg* moduleMsg)
{
    _module_msg = moduleMsg;
}

String ApiVlm::setup(ApiVlmSetupConfig_t config, String request_id)
{
    String cmd;
    {
        JsonDocument doc;
        doc["request_id"]              = request_id;
        doc["work_id"]                 = "vlm";
        doc["action"]                  = "setup";
        doc["object"]                  = "vlm.setup";
        doc["data"]["model"]           = config.model;
        doc["data"]["response_format"] = config.response_format;
        doc["data"]["enoutput"]        = config.enoutput;
        doc["data"]["enkws"]           = config.enkws;
        doc["data"]["max_token_len"]   = config.max_token_len;
        doc["data"]["prompt"]          = config.prompt;
        if (llm_version == "v1.0") {
            doc["data"]["model"] = "qwen2.5-0.5b";
            doc["data"]["input"] = config.input[0];
        } else {
            JsonArray inputArray = doc["data"]["input"].to<JsonArray>();
            for (const String& str : config.input) {
                inputArray.add(str);
            }
        }
        float version = llm_version.substring(1).toFloat();
        if (version >= 1.6) doc["data"]["model"] = "internvl2.5-1B-364-ax630c";

        for (const auto& pair : config.extra_params) {
            const String& key   = pair.first;
            const String& value = pair.second;

            if (value == "bool_true") {
                doc["data"][key] = true;
            } else if (value == "bool_false") {
                doc["data"][key] = false;
            } else if (value.indexOf('.') != -1) {
                doc["data"][key] = value.toFloat();
            } else if (value.length() > 0 && (isDigit(value.charAt(0)) || value.charAt(0) == '-')) {
                doc["data"][key] = value.toInt();
            } else {
                doc["data"][key] = value;
            }
        }
        serializeJson(doc, cmd);
    }

    String llm_work_id;
    _module_msg->sendCmdAndWaitToTakeMsg(
        cmd.c_str(), request_id,
        [&llm_work_id](ResponseMsg_t& msg) {
            // Copy work id
            llm_work_id = msg.work_id;
        },
        30000);
    return llm_work_id;
}

String ApiVlm::exit(String work_id, String request_id)
{
    String cmd;
    {
        JsonDocument doc;
        doc["request_id"] = request_id;
        doc["work_id"]    = work_id;
        doc["action"]     = "exit";
        serializeJson(doc, cmd);
    }

    _module_msg->sendCmdAndWaitToTakeMsg(
        cmd.c_str(), request_id,
        [&work_id](ResponseMsg_t& msg) {
            // Copy work id
            work_id = msg.work_id;
        },
        100);
    return work_id;
}

int ApiVlm::inference(String work_id, String input, String request_id)
{
    String cmd;
    {
        JsonDocument doc;
        doc["request_id"]     = request_id;
        doc["work_id"]        = work_id;
        doc["action"]         = "inference";
        doc["object"]         = "vlm.utf-8.stream";
        doc["data"]["delta"]  = input;
        doc["data"]["index"]  = 0;
        doc["data"]["finish"] = true;
        serializeJson(doc, cmd);
    }

    _module_msg->sendCmd(cmd.c_str());
    return MODULE_LLM_OK;
}

int ApiVlm::inference(String& work_id, uint8_t* input, size_t& raw_len, String request_id)
{
    String cmd;
    {
        JsonDocument doc;
        doc["RAW"]        = raw_len;
        doc["request_id"] = request_id;
        doc["work_id"]    = work_id;
        doc["action"]     = "inference";
        doc["object"]     = "cv.jpeg.base64";
        serializeJson(doc, cmd);
    }

    _module_msg->sendCmd(cmd.c_str());
    _module_msg->sendRaw(input, raw_len);
    return MODULE_LLM_OK;
}

int ApiVlm::inferenceAndWaitResult(String work_id, String input, std::function<void(String&)> onResult,
                                   uint32_t timeout, String request_id)
{
    inference(work_id, input, request_id);

    uint32_t time_out_count = millis();
    bool is_time_out        = false;
    bool is_msg_finish      = false;
    while (1) {
        _module_msg->update();
        _module_msg->takeMsg(request_id, [&time_out_count, &is_msg_finish, &onResult](ResponseMsg_t& msg) {
            String response_msg;
            {
                JsonDocument doc;
                deserializeJson(doc, msg.raw_msg);
                response_msg = doc["data"]["delta"].as<String>();
                if (!doc["data"]["finish"].isNull()) {
                    is_msg_finish = doc["data"]["finish"];
                    if (is_msg_finish) {
                        response_msg += '\n';
                    }
                }
            }
            if (onResult) {
                onResult(response_msg);
            }
            time_out_count = millis();
        });

        if (is_msg_finish) {
            break;
        }

        if (millis() - time_out_count > timeout) {
            is_time_out = true;
            break;
        }
    }

    if (is_time_out) {
        return MODULE_LLM_WAIT_RESPONSE_TIMEOUT;
    }
    return MODULE_LLM_OK;
}
