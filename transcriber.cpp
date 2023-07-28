//
// Created by j on 26/07/23.
//

#include "transcriber.h"


#include <iostream>
#include <sstream>
#include <vector>
#include <stack>
#include <thread>
#include <mutex>
#include <cmath>

int timestampToSample(int64_t t, int n_samples) {
    return std::max(0, std::min((int) n_samples - 1, (int) ((t*WHISPER_SAMPLE_RATE)/100)));
}

std::string estimate_diarization_speaker(std::vector<std::vector<float>> pcmf32s, int64_t t0, int64_t t1, bool id_only = false) {
    std::string speaker;
    const int64_t n_samples = pcmf32s[0].size();

    const int64_t is0 = timestampToSample(t0, n_samples);
    const int64_t is1 = timestampToSample(t1, n_samples);

    double energy0 = 0.0f;
    double energy1 = 0.0f;

    for (int64_t j = is0; j < is1; j++) {
        energy0 += fabs(pcmf32s[0][j]);
        energy1 += fabs(pcmf32s[1][j]);
    }

    if (energy0 > 1.1*energy1) {
        speaker = "0";
    } else if (energy1 > 1.1*energy0) {
        speaker = "1";
    } else {
        speaker = "?";
    }

    //printf("is0 = %lld, is1 = %lld, energy0 = %f, energy1 = %f, speaker = %s\n", is0, is1, energy0, energy1, speaker.c_str());

    if (!id_only) {
        speaker.insert(0, "(speaker ");
        speaker.append(")");
    }

    return speaker;
}


std::string output_json(struct whisper_context * ctx, const TranscribeParams & params) {
    std::stringstream jsonStream;
    int indent = 0;

    auto doindent = [&]() {
        for (int i = 0; i < indent; i++)
            jsonStream << "\t";
    };

    auto start_arr = [&](const char *name) {
        doindent();
        jsonStream << "\"" << name << "\": [\n";
        indent++;
    };

    auto end_arr = [&](bool end) {
        indent--;
        doindent();
        jsonStream << (end ? "]\n" : "},\n");
    };

    auto start_obj = [&](const char *name) {
        doindent();
        if (name) {
            jsonStream << "\"" << name << "\": {\n";
        } else {
            jsonStream << "{\n";
        }
        indent++;
    };

    auto end_obj = [&](bool end) {
        indent--;
        doindent();
        jsonStream << (end ? "}\n" : "},\n");
    };

    auto start_value = [&](const char *name) {
        doindent();
        jsonStream << "\"" << name << "\": ";
    };

    auto value_s = [&](const char *name, const char *val, bool end) {
        start_value(name);
        std::string val_escaped = Utils::escapeDoubleQuotesAndBackslashes(val);
        jsonStream << "\"" << val_escaped << (end ? "\"\n" : "\",\n");
    };

    auto end_value = [&](bool end) {
        jsonStream << (end ? "\n" : ",\n");
    };

    auto value_i = [&](const char *name, const int64_t val, bool end) {
        start_value(name);
        jsonStream << val;
        end_value(end);
    };

    auto value_b = [&](const char *name, const bool val, bool end) {
        start_value(name);
        jsonStream << (val ? "true" : "false");
        end_value(end);
    };

    start_obj(nullptr);
    value_s("systeminfo", whisper_print_system_info(), false);
    start_obj("model");
    value_s("type", whisper_model_type_readable(ctx), false);
    value_b("multilingual", whisper_is_multilingual(ctx), false);
    value_i("vocab", whisper_model_n_vocab(ctx), false);
    start_obj("audio");
    value_i("ctx", whisper_model_n_audio_ctx(ctx), false);
    value_i("state", whisper_model_n_audio_state(ctx), false);
    value_i("head", whisper_model_n_audio_head(ctx), false);
    value_i("layer", whisper_model_n_audio_layer(ctx), true);
    end_obj(false);
    start_obj("text");
    value_i("ctx", whisper_model_n_text_ctx(ctx), false);
    value_i("state", whisper_model_n_text_state(ctx), false);
    value_i("head", whisper_model_n_text_head(ctx), false);
    value_i("layer", whisper_model_n_text_layer(ctx), true);
    end_obj(false);
    value_i("mels", whisper_model_n_mels(ctx), false);
    value_i("ftype", whisper_model_ftype(ctx), true);
    end_obj(false);
    start_obj("params");
    value_s("model", params.model.c_str(), false);
    value_s("language", params.language.c_str(), false);
    value_b("translate", params.translate, true);
    end_obj(false);
    start_obj("result");
    value_s("language", whisper_lang_str(whisper_full_lang_id(ctx)), true);
    end_obj(false);
    start_arr("transcription");

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);

        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        start_obj(nullptr);
        start_obj("timestamps");
        value_s("from", Utils::toTimestamp(t0, true).c_str(), false);
        value_s("to", Utils::toTimestamp(t1, true).c_str(), true);
        end_obj(false);
        start_obj("offsets");
        value_i("from", t0 * 10, false);
        value_i("to", t1 * 10, true);
        end_obj(false);
        end_obj(i == (n_segments - 1));
    }

    end_arr(true);
    end_obj(true);
    return jsonStream.str();;
}



// Implement your item's constructor and methods here
TranscribeWorker::TranscribeWorker() {

}

TranscribeWorker::~TranscribeWorker() {
    if (context == nullptr) {
        return;
    }
    whisper_free(context);
}

std::string TranscribeWorker::Transcribe(TranscribeParams &params, std::vector<float> pcmf32, std::vector<std::vector<float>> &pcmf32s) {

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.strategy = params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;

    wparams.print_realtime   = false;
    wparams.print_progress   = false;
    wparams.print_timestamps = !params.no_timestamps;
    wparams.print_special    = false;
    wparams.translate        = params.translate;
    wparams.language         = params.language.c_str();
    wparams.detect_language  = params.detect_language;
    wparams.n_threads        = params.n_threads;
    wparams.n_max_text_ctx   = params.max_context >= 0 ? params.max_context : wparams.n_max_text_ctx;
    wparams.offset_ms        = params.offset_t_ms;
    wparams.duration_ms      = params.duration_ms;

    wparams.token_timestamps = params.max_len > 0;
    wparams.thold_pt         = params.word_thold;
    wparams.max_len          = params.max_len == 0 ? 60 : params.max_len;
    wparams.split_on_word    = params.split_on_word;

    wparams.speed_up         = params.speed_up;

    wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]

    wparams.initial_prompt   = params.prompt.c_str();

    wparams.greedy.best_of        = params.best_of;
    wparams.beam_search.beam_size = params.beam_size;

    wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;
    wparams.entropy_thold    = params.entropy_thold;
    wparams.logprob_thold    = params.logprob_thold;


    int transcription_result = whisper_full_parallel(context, wparams, pcmf32.data(), pcmf32.size(), params.n_processors);
    if( transcription_result != 0) {
        throw TranscribeException("Failed to transcribe audio");
    }

    return output_json(context,  params);
}

void TranscribeWorker::Initialize(TranscribeParams &params) {

    context = whisper_init_from_file(params.model.c_str());

    if (context == nullptr) {
        throw TranscribeInitException( "failed to initialize whisper context");
    }

    whisper_ctx_init_openvino_encoder(context, nullptr, params.openvino_encode_device.c_str(), nullptr);
}

TranscriberPool::TranscriberPool(std::size_t poolSize, TranscribeParams params) {
    // Fill the pool with reusable items
    for (std::size_t i = 0; i < poolSize; ++i) {
        auto wrkr = new TranscribeWorker();
        wrkr->Initialize(params);
        pool.push(wrkr);
    }
}

TranscriberPool::~TranscriberPool() {
    // Deallocate all objects when the pool is destroyed
    while (!pool.empty()) {
        delete pool.top();
        pool.pop();
    }
}

TranscribeWorker *TranscriberPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex);

    // Wait until an item is available in the pool
    while (pool.empty()) {
        cv.wait(lock);
    }

    TranscribeWorker *worker = pool.top();
    pool.pop();
    return worker;
}

void TranscriberPool::release(TranscribeWorker *worker) {
    std::unique_lock<std::mutex> lock(mutex);
    pool.push(worker);
    // Notify waiting threads that an item is available in the pool
    cv.notify_one();
}

