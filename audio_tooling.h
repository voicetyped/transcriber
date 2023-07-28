//
// Created by j on 27/07/23.
//

#ifndef TRANSCRIBER_AUDIO_TOOLING_H
#define TRANSCRIBER_AUDIO_TOOLING_H


#include <vector>


class AudioTooling {

public:
    static void resampleAudioFile(const std::string& inputFileName, const std::string& outputFileName);

    static void preProcessWav(const std::string &waveFileName, std::vector<float> &pcmf32,
                                   std::vector<std::vector<float>> &pcmf32s, bool stereo);

    static std::string outputFileRename(std::string inputFileName) {

        // Find the position of the file extension (e.g., ".mp3")
        size_t dotPos = inputFileName.find_last_of('.');
        if (dotPos == std::string::npos) {
            return inputFileName;
        }

        // Get the filename without the extension
        std::string fileNameWithoutExt = inputFileName.substr(0, dotPos);

        // Create the output filename with the "_resample.wav" extension
        return fileNameWithoutExt + "_resampled.wav";
    }

};


class WaveToFloatException : public std::exception {
public:
    explicit WaveToFloatException(std::string message) : msg(std::move(message)) {}

    [[nodiscard]] const char *what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};


class ResamplingException : public std::exception {
public:
    explicit ResamplingException(std::string message) : msg(std::move(message)) {}

    [[nodiscard]] const char *what() const noexcept override {
        return msg.c_str();
    }

private:
    std::string msg;
};


#endif //TRANSCRIBER_AUDIO_TOOLING_H
