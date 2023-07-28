//
// Created by j on 27/07/23.
//

// use your favorite implementations
#define DR_WAV_IMPLEMENTATION

#include "dr_wav.h"
#include <iostream>
#include <string>
#include <memory>

#include "audio_tooling.h"

#define COMMON_SAMPLE_RATE 16000


void AudioTooling::resampleAudioFile(const std::string& inputFileName, const std::string& outputFileName){

    std::string ffmpegCommand = "ffmpeg -i " + inputFileName + " -loglevel panic -ar 16000 -ac 1 -acodec pcm_s16le " + outputFileName;
    int result = std::system(ffmpegCommand.c_str());
    if( result != 0)
    {
        throw ResamplingException("Error resampling input file with code : "+ std::to_string(result));
    }
}

void AudioTooling::preProcessWav(const std::string &waveFileName, std::vector<float> &pcmf32,
                                 std::vector<std::vector<float>> &pcmf32s, bool stereo) {

    drwav wav;

    if (drwav_init_file(&wav, waveFileName.c_str(), nullptr) == false) {
        throw WaveToFloatException("failed to open WAV file : " + waveFileName);
    }

    if (wav.channels != 1 && wav.channels != 2) {
        throw WaveToFloatException("WAV file : " + waveFileName + " must be mono or stereo ");
    }

    if (stereo && wav.channels != 2) {
        throw WaveToFloatException("WAV file : " + waveFileName + " must be stereo for diarization");
    }

    if (wav.sampleRate != COMMON_SAMPLE_RATE) {
        throw WaveToFloatException(
                "WAV file : " + waveFileName + " must be " + std::to_string(COMMON_SAMPLE_RATE / 1000) + " kHz");
    }

    if (wav.bitsPerSample != 16) {
        throw WaveToFloatException("WAV file : " + waveFileName + "  must be 16-bit");
    }

    const uint64_t n = wav.totalPCMFrameCount;

    std::vector<int16_t> pcm16;
    pcm16.resize(n * wav.channels);
    drwav_read_pcm_frames_s16(&wav, n, pcm16.data());
    drwav_uninit(&wav);

    // convert to mono, float
    pcmf32.resize(n);
    if (wav.channels == 1) {
        for (uint64_t i = 0; i < n; i++) {
            pcmf32[i] = float(pcm16[i]) / 32768.0f;
        }
    } else {
        for (uint64_t i = 0; i < n; i++) {
            pcmf32[i] = float(pcm16[2 * i] + pcm16[2 * i + 1]) / 65536.0f;
        }
    }

    if (stereo) {
        // convert to stereo, float
        pcmf32s.resize(2);

        pcmf32s[0].resize(n);
        pcmf32s[1].resize(n);
        for (uint64_t i = 0; i < n; i++) {
            pcmf32s[0][i] = float(pcm16[2 * i]) / 32768.0f;
            pcmf32s[1][i] = float(pcm16[2 * i + 1]) / 32768.0f;
        }
    }
}





