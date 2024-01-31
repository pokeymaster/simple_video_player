#include <SDL.h>
#include <SDL_video.h>
#include <iostream>
#include <string>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return -1;
    }

    const char* videoFile = argv[1];

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("Video Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Initialize FFmpeg
    av_register_all();
    avformat_network_init();

    // Open the video file
    AVFormatContext* formatContext = avformat_alloc_context();
    if (avformat_open_input(&formatContext, videoFile, NULL, NULL) != 0) {
        std::cerr << "Could not open the video file." << std::endl;
        return -1;
    }

    // Retrieve stream information
    if (avformat_find_stream_info(formatContext, NULL) < 0) {
        std::cerr << "Could not find stream information." << std::endl;
        return -1;
    }

    // Find the first video stream
    int videoStreamIndex = -1;
    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Could not find a video stream." << std::endl;
        return -1;
    }

    AVCodecParameters* codecParameters = formatContext->streams[videoStreamIndex]->codecpar;

    // Find the decoder for the video stream
    AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
    if (!codec) {
        std::cerr << "Unsupported codec." << std::endl;
        return -1;
    }

    // Create a codec context
    AVCodecContext* codecContext = avcodec_alloc_context3(codec);
    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
        std::cerr << "Failed to copy codec parameters." << std::endl;
        return -1;
    }

    // Open the codec
    if (avcodec_open2(codecContext, codec, NULL) < 0) {
        std::cerr << "Could not open the codec." << std::endl;
        return -1;
    }

    // Allocate video frame
    AVFrame* frame = av_frame_alloc();
    if (!frame) {
        std::cerr << "Could not allocate video frame." << std::endl;
        return -1;
    }

    // Allocate an AVPacket
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        std::cerr << "Could not allocate packet." << std::endl;
        return -1;
    }

    // Main loop
    bool quit = false;
    SDL_Event e;

    // Variables for handling time and seeking
    int64_t start_time = av_gettime();
    int64_t frame_duration = av_rescale_q(1, { 1, AV_TIME_BASE }, formatContext->streams[videoStreamIndex]->time_base);

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_SPACE: // Play/Pause
                        SDL_PauseAudio(1 - SDL_GetAudioStatus());
                        break;
                    case SDLK_LEFT: // Seek backward
                        av_seek_frame(formatContext, videoStreamIndex, frame->pts - frame_duration * 10, AVSEEK_FLAG_BACKWARD);
                        break;
                    case SDLK_RIGHT: // Seek forward
                        av_seek_frame(formatContext, videoStreamIndex, frame->pts + frame_duration * 10, 0);
                        break;
                    default:
                        break;
                }
            }
        }

        // Read a frame from the video stream
        if (av_read_frame(formatContext, packet) >= 0) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecContext, packet) == 0) {
                    while (avcodec_receive_frame(codecContext, frame) == 0) {
                        // Clear the renderer
                        SDL_RenderClear(renderer);

                        // Create a texture from the frame
                        SDL_Texture* videoTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, frame->width, frame->height);
                        if (!videoTexture) {
                            std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
                            return -1;
                        }

                        // Update the texture with the frame data
                        SDL_UpdateYUVTexture(videoTexture, NULL, frame->data[0], frame->linesize[0], frame->data[1], frame->linesize[1], frame->data[2], frame->linesize[2]);

                        // Render the texture
                        SDL_RenderCopy(renderer, videoTexture, NULL, NULL);

                        // Update the renderer
                        SDL_RenderPresent(renderer);

                        // Delay to maintain the video playback speed
                        int64_t pts = av_frame_get_best_effort_timestamp(frame);
                        int64_t delay = av_gettime() - start_time - pts * av_q2d(formatContext->streams[videoStreamIndex]->time_base) * 1000000;

                        if (delay > 0) {
                            SDL_Delay((Uint32)delay / 1000);
                        }

                        SDL_DestroyTexture(videoTexture);
                    }
                }
            }

            av_packet_unref(packet);
        }
    }

    // Cleanup
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecContext);
    avformat_close_input(&formatContext);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
