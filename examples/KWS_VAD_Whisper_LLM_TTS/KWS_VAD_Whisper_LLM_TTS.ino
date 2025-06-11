/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleLLM.h>

M5ModuleLLM module_llm;

/* Must be capitalized */
String wake_up_keyword = "HELLO";
// String wake_up_keyword = "你好你好";
String kws_work_id;
String vad_work_id;
String whisper_work_id;
String llm_work_id;
String melotts_work_id;
String language;

void setup()
{
    M5.begin();
    M5.Display.setTextSize(2);
    M5.Display.setTextScroll(true);
    // M5.Display.setFont(&fonts::efontCN_12);  // Support Chinese display
    // M5.Display.setFont(&fonts::efontJA_12);  // Support Japanese display

    language = "en_US";
    // language = "zh_CN";
    // language = "ja_JP";

    /* Init module serial port */
    // int rxd = 16, txd = 17;  // Basic
    // int rxd = 13, txd = 14;  // Core2
    // int rxd = 18, txd = 17;  // CoreS3
    int rxd = M5.getPin(m5::pin_name_t::port_c_rxd);
    int txd = M5.getPin(m5::pin_name_t::port_c_txd);
    Serial2.begin(115200, SERIAL_8N1, rxd, txd);

    /* Init module */
    module_llm.begin(&Serial2);

    /* Make sure module is connected */
    M5.Display.printf(">> Check ModuleLLM connection..\n");
    while (1) {
        if (module_llm.checkConnection()) {
            break;
        }
    }

    /* Reset ModuleLLM */
    M5.Display.printf(">> Reset ModuleLLM..\n");
    module_llm.sys.reset();

    /* Setup Audio module */
    M5.Display.printf(">> Setup audio..\n");
    module_llm.audio.setup();

    /* Setup KWS module and save returned work id */
    M5.Display.printf(">> Setup kws..\n");
    m5_module_llm::ApiKwsSetupConfig_t kws_config;
    kws_config.kws = wake_up_keyword;
    kws_work_id    = module_llm.kws.setup(kws_config, "kws_setup", language);

    /* Setup VAD module and save returned work id */
    M5.Display.printf(">> Setup vad..\n");
    m5_module_llm::ApiVadSetupConfig_t vad_config;
    vad_config.input = {"sys.pcm", kws_work_id};
    vad_work_id      = module_llm.vad.setup(vad_config, "vad_setup");

    /* Setup Whisper module and save returned work id */
    M5.Display.printf(">> Setup whisper..\n");
    m5_module_llm::ApiWhisperSetupConfig_t whisper_config;
    whisper_config.input    = {"sys.pcm", kws_work_id, vad_work_id};
    whisper_config.language = "en";
    // whisper_config.language = "zh";
    // whisper_config.language = "ja";
    whisper_work_id = module_llm.whisper.setup(whisper_config, "whisper_setup");

    M5.Display.printf(">> Setup llm..\n");
    llm_work_id = module_llm.llm.setup();

    M5.Display.printf(">> Setup melotts..\n\n");
    m5_module_llm::ApiMelottsSetupConfig_t melotts_config;
    melotts_config.input = {"tts.utf-8.stream", llm_work_id};
    melotts_work_id      = module_llm.melotts.setup(melotts_config, "melotts_setup", language);

    M5.Display.printf(">> Setup ok\n>> Say \"%s\" to wakeup\n", wake_up_keyword.c_str());
}

void loop()
{
    /* Update ModuleLLM */
    module_llm.update();

    /* Handle module response messages */
    for (auto& msg : module_llm.msg.responseMsgList) {
        /* If KWS module message */
        if (msg.work_id == kws_work_id) {
            M5.Display.setTextColor(TFT_GREENYELLOW);
            M5.Display.printf(">> Keyword detected\n");
        }

        if (msg.work_id == vad_work_id) {
            M5.Display.setTextColor(TFT_GREENYELLOW);
            M5.Display.printf(">> vad detected\n");
        }
        /* If ASR module message */
        if (msg.work_id == whisper_work_id) {
            /* Check message object type */
            if (msg.object == "asr.utf-8") {
                /* Parse message json and get ASR result */
                JsonDocument doc;
                deserializeJson(doc, msg.raw_msg);
                String asr_result = doc["data"].as<String>();

                M5.Display.setTextColor(TFT_YELLOW);
                M5.Display.printf(">> %s\n", asr_result.c_str());

                module_llm.llm.inferenceAndWaitResult(llm_work_id, asr_result.c_str(), [](String& result) {
                    /* Show result on screen */
                    handleLLMResult(result);
                });
            }
        }
    }

    /* Clear handled messages */
    module_llm.msg.responseMsgList.clear();
}

void handleLLMResult(String& result)
{
    M5.Display.printf("%s", result.c_str());
}
