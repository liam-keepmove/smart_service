/*
 进行音频采集，采集pcm数据并直接保存pcm数据
 音频参数：
   声道数：   1
   采样位数：  16bit、LE格式
   采样频率：  44100Hz
运行示例:
$ gcc linux_pcm_save.c -lasound
$ ./a.out hw:0 123.pcm
*/

#include <alsa/asoundlib.h>
#include <atomic>
#include <iostream>
#include <signal.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <stdlib.h>
#include <zmq.hpp>

#define AudioFormat SND_PCM_FORMAT_S16_LE // 指定音频的格式,其他常用格式：SND_PCM_FORMAT_U24_LE、SND_PCM_FORMAT_U32_LE
#define AUDIO_CHANNEL_SET 1               // 1单声道   2立体声
#define AUDIO_RATE_SET 44100              // 音频采样率,常用的采样频率: 44100Hz 、16000HZ、8000HZ、48000HZ、22050HZ

std::atomic_bool stop_flag(false);
void exit_sighandler(int sig) {
    stop_flag = true;
}

int main(int argc, char* argv[]) {
    int i;
    int err;
    char* buffer;
    int buffer_frames = AUDIO_RATE_SET; // buffer的帧数设置为音频流的帧数,意思是1s获取一次,延时1s
    unsigned int rate = AUDIO_RATE_SET;
    snd_pcm_t* capture_handle;      // 一个指向PCM设备的句柄
    snd_pcm_hw_params_t* hw_params; // 此结构包含有关硬件的信息，可用于指定PCM流的配置
    zmq::context_t context(1);
    zmq::socket_t puber(context, zmq::socket_type::pub);
    puber.bind("tcp://*:56666");

    std::shared_ptr<spdlog::logger> logger = nullptr;
    try {
        logger = spdlog::basic_logger_mt("basic_logger", "logs/sound_zmq_log.txt");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return 1;
    }

    /*注册信号捕获退出接口*/
    signal(SIGINT, exit_sighandler);
    signal(SIGTERM, exit_sighandler);

    /*PCM的采样格式在pcm.h文件里有定义*/
    snd_pcm_format_t format = AudioFormat; // 采样位数：16bit、LE格式

    /*打开音频采集卡硬件，并判断硬件是否打开成功，若打开失败则打印出错误提示*/
    // SND_PCM_STREAM_PLAYBACK 输出流
    // SND_PCM_STREAM_CAPTURE  输入流
    // 参数4: 0(阻塞模式打开流) or SND_PCM_NONBLOCK(流读取不阻塞) or SND_PCM_ASYNC(异步读取流)
    if ((err = snd_pcm_open(&capture_handle, argv[1], SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        logger->error("无法打开音频设备: {} ({})", argv[1], snd_strerror(err));
        return 1;
    }
    logger->info("音频接口打开成功");

    /*分配硬件参数结构对象，并判断是否分配成功*/
    if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
        logger->error("无法分配硬件参数结构 ({})", snd_strerror(err));
        return 1;
    }
    logger->info("硬件参数结构已分配成功.");

    /*按照默认设置对硬件对象进行设置，并判断是否设置成功*/
    if ((err = snd_pcm_hw_params_any(capture_handle, hw_params)) < 0) {
        logger->error("无法初始化硬件参数结构 ({})", snd_strerror(err));
        return 1;
    }
    logger->info("硬件参数结构初始化成功.");

    /*
      设置数据为交叉模式，并判断是否设置成功
      interleaved/non interleaved:交叉/非交叉模式。
      表示在多声道数据传输的过程中是采样交叉的模式还是非交叉的模式。
      对多声道数据，如果采样交叉模式，使用一块buffer即可，其中各声道的数据交叉传输；
      如果使用非交叉模式，需要为各声道分别分配一个buffer，各声道数据分别传输。
    */
    if ((err = snd_pcm_hw_params_set_access(capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        logger->error("无法设置访问类型({})", snd_strerror(err));
        return 1;
    }
    logger->info("访问类型设置成功.");

    /*设置数据编码格式，并判断是否设置成功*/
    if ((err = snd_pcm_hw_params_set_format(capture_handle, hw_params, format)) < 0) {
        logger->error("无法设置格式 ({})", snd_strerror(err));
        return 1;
    }
    logger->info("PCM数据格式设置成功.");

    /*设置采样频率，并判断是否设置成功*/
    if ((err = snd_pcm_hw_params_set_rate_near(capture_handle, hw_params, &rate, 0)) < 0) {
        logger->error("无法设置采样率({})", snd_strerror(err));
        return 1;
    }
    logger->info("采样率设置成功");

    /*设置声道，并判断是否设置成功*/
    if ((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params, AUDIO_CHANNEL_SET)) < 0) {
        logger->error("无法设置声道数({})", snd_strerror(err));
        return 1;
    }
    logger->info("声道数设置成功.");

    /*将配置写入驱动程序中，并判断是否配置成功*/
    if ((err = snd_pcm_hw_params(capture_handle, hw_params)) < 0) {
        logger->error("无法向驱动程序设置参数({})", snd_strerror(err));
        return 1;
    }
    logger->info("参数设置成功.");

    /*准备音频接口,并判断是否准备好*/
    if ((err = snd_pcm_prepare(capture_handle)) < 0) {
        logger->error("无法使用音频接口 ({})", snd_strerror(err));
        return 1;
    }
    logger->info("音频接口准备好.");

    /*配置一个数据缓冲区用来缓冲数据*/
    // snd_pcm_format_width(format) 获取样本格式对应的大小(单位是:bit)
    int frame_byte = snd_pcm_format_width(format) / 8;
    buffer = (char*)malloc(buffer_frames * frame_byte * AUDIO_CHANNEL_SET);
    logger->info("缓冲区分配成功.");

    /*开始采集音频pcm数据*/
    logger->info("开始采集数据...");
    while (!stop_flag) {
        int rc = 0;
        /*从声卡设备阻塞读取一周期的音频数据*/
        if ((rc = snd_pcm_readi(capture_handle, buffer, buffer_frames)) < 0) {
            logger->error("从音频接口读取失败({})", snd_strerror(rc));
            return 1;
        }
        /*写数据到文件: 音频的每帧数据样本大小是1通道*2字节样本宽度*/
        // fwrite(buffer, (frame_byte * AUDIO_CHANNEL_SET), rc, pcm_data_file);
        static zmq::message_t msg;
        msg.rebuild(buffer, (AUDIO_CHANNEL_SET * frame_byte) * rc);
        logger->info("rc={} value={}", rc, (AUDIO_CHANNEL_SET * frame_byte) * rc);
        logger->info("send {} byte.", msg.size());
        // puber.send(msg, ZMQ_DONTWAIT);
        puber.send(msg);
    }

    logger->info("停止采集.");

    // 释放硬件参数malloc的数据结构
    snd_pcm_hw_params_free(hw_params);

    /*释放数据缓冲区*/
    free(buffer);

    /*关闭音频采集卡硬件*/
    snd_pcm_close(capture_handle);

    return 0;
}
