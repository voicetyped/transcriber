//
// Created by j on 26/07/23.
//

#ifndef TRANSCRIBER_UTILITIES_H
#define TRANSCRIBER_UTILITIES_H

#include <string>
#include <iostream>
#include <execinfo.h>


const static char *ENV_NUMBER_OF_PROCESSORS = "NUMBER_OF_PROCESSORS";
const static char *ENV_OFFSET_TIME_MILISEC = "ENV_OFFSET_TIME_MILISEC";
const static char *ENV_OFFSET_NUMBER = "ENV_OFFSET_NUMBER";
const static char *ENV_DURATION_MILISEC = "ENV_DURATION_MILISEC";
const static char *ENV_MAXIMUM_CONTEXT = "ENV_MAXIMUM_CONTEXT";
const static char *ENV_MAXIMUM_LENGTH = "ENV_MAXIMUM_LENGTH";
const static char *ENV_BEST_OF = "ENV_BEST_OF";
const static char *ENV_BEAM_SIZE = "ENV_BEAM_SIZE";
const static char *ENV_WORD_THRESHOLD = "ENV_WORD_THRESHOLD";
const static char *ENV_ENTROPY_THRESHOLD = "ENV_ENTROPY_THRESHOLD";
const static char *ENV_LOG_PROB_THRESHOLD = "ENV_LOG_PROB_THRESHOLD";
const static char *ENV_SPEED_UP = "ENV_SPEED_UP";
const static char *ENV_TRANSLATE = "ENV_TRANSLATE";
const static char *ENV_DIARIZE = "ENV_DIARIZE";
const static char *ENV_TINY_DIARIZE = "ENV_TINY_DIARIZE";
const static char *ENV_SPLIT_ON_WORD = "ENV_SPLIT_ON_WORD";
const static char *ENV_DEFAULT_LANGUAGE = "ENV_DEFAULT_LANGUAGE";
const static char *ENV_DETECT_LANGUAGE = "ENV_DETECT_LANGUAGE";
const static char *ENV_NO_FALLBACK = "ENV_NO_FALLBACK";
const static char *ENV_NO_TIMESTAMPS = "ENV_NO_TIMESTAMPS";
const static char *ENV_PROMPT = "ENV_PROMPT";
const static char *ENV_DEFAULT_MODEL = "ENV_DEFAULT_MODEL";
const static char *ENV_OPEN_VINO_ENCODER = "ENV_OPEN_VINO_ENCODER";
const static char *ENV_PATH_FOR_AUDIO_FILES = "ENV_PATH_FOR_AUDIO_FILES";


class Utils {
public:
    // Function to get the value of an environment variable or return a default value
    static std::string getEnvOrDefault(const char *env_var_name, const std::string &default_value) {
        const char *env_var_value = std::getenv(env_var_name);
        return env_var_value ? std::string(env_var_value) : default_value;
    }

    static int getEnvOrDefaultBool(const char *env_var_name, bool default_value) {

        std::string env = getEnvOrDefault(env_var_name, std::to_string(default_value));
        return env == "true" || env == "1" || env == "yes" || env == "on" || default_value;

    }

    static int getEnvOrDefaultInt(const char *env_var_name, int default_value) {

        try {
            std::string env = getEnvOrDefault(env_var_name, std::to_string(default_value));
            return std::stoi(env);
        } catch (const std::exception &e) {
            std::cerr << "Error converting string to integer: " << e.what() << std::endl;
            return default_value;
        }
    }

    static float getEnvOrDefaultFloat(const char *env_var_name, float default_value) {

        try {
            std::string env = getEnvOrDefault(env_var_name, std::to_string(default_value));
            return std::stof(env);
        } catch (const std::exception &e) {
            std::cerr << "Error converting string to integer: " << e.what() << std::endl;
            return default_value;
        }
    }

    static std::string getFilesStoragePath( std::string fileName ){

        std::string fileDirectory = getEnvOrDefault(ENV_PATH_FOR_AUDIO_FILES, "/tmp/");
                return fileDirectory.append(fileName);
    }

    static void logStackTrace() {
        const int max_frames = 50;
        void* frames[max_frames];
        int num_frames = backtrace(frames, max_frames);
        char** symbols = backtrace_symbols(frames, num_frames);

        std::cout << "Stack Trace:\n";
        for (int i = 0; i < num_frames; i++) {
            std::cout << symbols[i] << std::endl;
        }

        free(symbols);
    }

    static std::string escapeDoubleQuotesAndBackslashes(const char *str) {
        if (str == nullptr) {
            return {};
        }

        size_t escaped_length = 0;

        // Calculate the required size for the escaped string
        for (size_t i = 0; str[i] != '\0'; i++) {
            if (str[i] == '"' || str[i] == '\\') {
                escaped_length += 2; // Escaping requires 2 characters (e.g., \")
            } else {
                escaped_length++;
            }
        }

        // Construct the escaped string directly
        std::string escaped;
        escaped.reserve(escaped_length);

        for (size_t i = 0; str[i] != '\0'; i++) {
            if (str[i] == '"' || str[i] == '\\') {
                escaped.push_back('\\'); // Add escape character
            }
            escaped.push_back(str[i]);
        }

        return escaped;
    }

    //  500 -> 00:05.000
// 6000 -> 01:00.000
    static std::string toTimestamp(int64_t t, bool comma = false) {
        int64_t msec = t * 10;
        int64_t hr = msec / (1000 * 60 * 60);
        msec = msec - hr * (1000 * 60 * 60);
        int64_t min = msec / (1000 * 60);
        msec = msec - min * (1000 * 60);
        int64_t sec = msec / 1000;
        msec = msec - sec * 1000;

        char buf[32];
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int) hr, (int) min, (int) sec, comma ? "," : ".",
                 (int) msec);

        return {buf};
    }


};


#endif //TRANSCRIBER_UTILITIES_H
