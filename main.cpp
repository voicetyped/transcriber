//
//  sample.cc
//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

#include "utilities.h"
#include "transcriber.h"
#include "audio_tooling.h"
#include <cmath>
#include <xid/xid.h>

#include <chrono>
#include <cstdio>

#define CPPHTTPLIB_USE_POLL

#include "httplib.h"

#define SERVER_CERT_FILE "./cert.pem"
#define SERVER_PRIVATE_KEY_FILE "./key.pem"

using namespace httplib;
using namespace std;

#include <iostream>
#include <csignal>

// Signal handler function
void signalHandler(int signum) {
    std::cerr << "Segmentation fault occurred (Signal " << signum << ")" << std::endl;

    Utils::logStackTrace();
    // You can perform additional actions or logging here if needed.
    // Note: It's generally recommended to avoid doing complex operations inside a signal handler.
    exit(signum); // Terminate the program with the signal number as the exit code
}


int main() {

    // Register the signal handler for SIGSEGV
    signal(SIGSEGV, signalHandler);

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
#else
    Server svr;
#endif

    TranscribeParams params = TranscribeParams();

    const std::size_t poolSize = params.n_processors;
    TranscriberPool pool = TranscriberPool(poolSize, params);



    if (!svr.is_valid()) {
        printf("server has an error...\n");
        return -1;
    }

    svr.Get("/", [=](const Request & /*req*/, Response &res) {
        res.set_content("Say my name\n", "text/plain");
    });

    svr.Get("/stop",
            [&](const Request & /*req*/, Response & /*res*/) { svr.stop(); });

    svr.Post("/", [&pool, &params](const Request &req, Response &res) {

        std::vector<float> pcmf32;               // mono-channel F32 PCM
        std::vector<std::vector<float>> pcmf32s; // stereo-channel F32 PCM

        auto audioFile = req.get_file_value("audio_file");
        std::string audioInputFile = xid::next().string().append("_" + audioFile.filename);
        std::string audioOutputFile = AudioTooling::outputFileRename(audioInputFile);

        audioInputFile = Utils::getFilesStoragePath(audioInputFile);
        audioOutputFile = Utils::getFilesStoragePath(audioOutputFile);
        try {

            {
                ofstream ofs(audioInputFile, ios::binary);
                ofs << audioFile.content;
            }


            AudioTooling::resampleAudioFile(audioInputFile, audioOutputFile);

            AudioTooling::preProcessWav(audioOutputFile, pcmf32, pcmf32s, false);

            TranscribeWorker* worker = pool.acquire();
            std::string response = worker->Transcribe(params, pcmf32, pcmf32s);
            pool.release(worker);

            res.set_content(response, "text/json");


        } catch (const ResamplingException &e) {
            std::string error_message = "{\"error\":\"could not transcribe file\", \"reason\":\"";
            error_message = error_message.append(e.what()).append("\"}");
            res.set_content(error_message, "text/json");


            std::cerr << "Resampling Exception: " << e.what() << std::endl;
            Utils::logStackTrace();

        } catch (const std::exception &e) {

            std::string error_message = "{\"error\":\"could not transcribe file\", \"reason\":\"";
            error_message = error_message.append(e.what()).append("\"}");
            res.set_content(error_message, "text/json");

            std::cerr << "Exception occurred: " << e.what() << std::endl;
            Utils::logStackTrace();
        } catch (...) {

            std::string error_message = "{\"error\":\"could not transcribe file\", \"reason\":\"error is unknown\"}";

            res.set_content(error_message, "text/json");

            std::cerr << "Unknown exception occurred." << std::endl;
            Utils::logStackTrace();
        }

        if (std::remove(audioInputFile.c_str()) != 0) {
            std::perror("Error deleting input file");
        }

        if (std::remove(audioOutputFile.c_str()) != 0) {
            std::perror("Error deleting output file");
        }

    });


    svr.set_error_handler([](const Request & /*req*/, Response &res) {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });

    svr.set_payload_max_length(1024 * 1024 * 128);


    int port_value = Utils::getEnvOrDefaultInt("PORT", 8080);

    std::cout << "Starting up server on port : " << port_value << std::endl;
    svr.listen("0.0.0.0", port_value);

    return 0;
}