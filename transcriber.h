//
// Created by j on 26/07/23.
//

#ifndef TRANSCRIBER_TRANSCRIBER_H
#define TRANSCRIBER_TRANSCRIBER_H


#pragma once

#include <stack>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <utility>
#include <vector>
#include "utilities.h"
#include "whisper.h"


// processing parameters
struct TranscribeParams {
    int32_t n_threads = std::max(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors = Utils::getEnvOrDefaultInt(ENV_NUMBER_OF_PROCESSORS,
                                                     (int32_t) std::thread::hardware_concurrency());
    int32_t offset_t_ms = Utils::getEnvOrDefaultInt(ENV_NUMBER_OF_PROCESSORS, 1);
    int32_t offset_n = Utils::getEnvOrDefaultInt(ENV_OFFSET_NUMBER, 0);
    int32_t duration_ms = Utils::getEnvOrDefaultInt(ENV_DURATION_MILISEC, 0);
    int32_t max_context = -Utils::getEnvOrDefaultInt(ENV_MAXIMUM_CONTEXT, -1);
    int32_t max_len = Utils::getEnvOrDefaultInt(ENV_MAXIMUM_LENGTH, 0);
    int32_t best_of = Utils::getEnvOrDefaultInt(ENV_BEST_OF, 2);
    int32_t beam_size = Utils::getEnvOrDefaultInt(ENV_BEAM_SIZE, -1);

    float word_thold = Utils::getEnvOrDefaultFloat(ENV_WORD_THRESHOLD, 0.01f);
    float entropy_thold = Utils::getEnvOrDefaultFloat(ENV_ENTROPY_THRESHOLD, 2.40f);
    float logprob_thold = Utils::getEnvOrDefaultFloat(ENV_LOG_PROB_THRESHOLD, -1.00f);

    bool speed_up = Utils::getEnvOrDefaultBool(ENV_SPEED_UP, false);
    bool translate = Utils::getEnvOrDefaultBool(ENV_TRANSLATE, false);
    bool detect_language = Utils::getEnvOrDefaultBool(ENV_DETECT_LANGUAGE, false);
    bool diarize = Utils::getEnvOrDefaultBool(ENV_DIARIZE, false);
    bool tinydiarize = Utils::getEnvOrDefaultBool(ENV_TINY_DIARIZE, false);
    bool split_on_word = Utils::getEnvOrDefaultBool(ENV_SPLIT_ON_WORD, false);
    bool no_fallback = Utils::getEnvOrDefaultBool(ENV_NO_FALLBACK, false);
    bool no_timestamps = Utils::getEnvOrDefaultBool(ENV_NO_TIMESTAMPS, false);


    std::string prompt = Utils::getEnvOrDefault(ENV_PROMPT, "");
    std::string language = Utils::getEnvOrDefault(ENV_DEFAULT_LANGUAGE, "en");
    std::string model = Utils::getEnvOrDefault(ENV_DEFAULT_MODEL, "/home/j/.cache/whisper/ggml-base.en.bin");

    std::string openvino_encode_device = Utils::getEnvOrDefault(ENV_OPEN_VINO_ENCODER, "CPU");

    std::vector<std::string> fname_inp = {};
    std::vector<std::string> fname_out = {};
};

class TranscribeWorker {
public:
    TranscribeWorker();

    ~TranscribeWorker();

    void Initialize(TranscribeParams &params);

    std::string
    Transcribe(TranscribeParams &params, std::vector<float> pcmf32, std::vector<std::vector<float>> &pcmf32s);

private:
    whisper_context *context;
};

class TranscriberPool {
public:
    explicit TranscriberPool(std::size_t poolSize, TranscribeParams params);

    ~TranscriberPool();

    TranscribeWorker *acquire();

    void release(TranscribeWorker *worker);

private:
    std::stack<TranscribeWorker *> pool;
    std::mutex mutex;
    std::condition_variable cv;
};

class TranscribeInitException : public std::exception {
public:
    explicit TranscribeInitException(std::string message) : msg(std::move(message)) {}

    [[nodiscard]] const char *what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

class TranscribeException : public std::exception {
public:
    explicit TranscribeException(std::string message) : msg(std::move(message)) {}

    [[nodiscard]] const char *what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};

#endif //TRANSCRIBER_TRANSCRIBER_H
