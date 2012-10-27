#ifndef ROXLU_VIDEORECORDERH
#define ROXLU_VIDEORECORDERH

#define CONVERTER_YUV 0
#define CONVERTER_SWSCALE 1
//#define RGB_CONVERTER CONVERTER_YUV
#define RGB_CONVERTER CONVERTER_SWSCALE

extern "C" {
#if RGB_CONVERTER == CONVERTER_SWSCALE
#include <libswscale/swscale.h>
#else
#include <libyuv.h> 
#endif

#include <x264.h>
#include <speex/speex.h>
}

#include <videorecorder/VideoTypes.h>
#include <videorecorder/VideoIO.h>
#include <videorecorder/VideoIOFLV.h>
#include <videorecorder/VideoIOFLVSocket.h>

class VideoRecorder {
public:
#if RGB_CONVERTER == CONVERTER_SWSCALE
	VideoRecorder(int inW, int inH, int outW, int outH, int fps);
#else
	VideoRecorder(int inW, int inH, int fps);
#endif
	~VideoRecorder();
	bool openFile(const char* filepath);
	int addVideoFrame(unsigned char* pixels);
	int addAudioFrame(short int* data, int size);
	void closeFile();
	void setIO(VideoIO* io);
	void setFPS(int fps);
	void writeIOHeaders(); // tmp testing
private:
	void initEncoders();
	void initVideoEncoder();
	void initAudioEncoder();
	void setParams();
	int writeHeaders();
	void writePackets();
private:
	int out_width;
	int out_height;
	int in_width;
	int in_height;
	int in_stride;
	int fps;
	int num_frames;
	bool vfr_input; // variable frame rate
	rx_uint64 start_time;

	x264_param_t params;
	x264_t* encoder;
	x264_picture_t pic_in;
	x264_picture_t pic_out;
	x264_nal_t* nals;
	int num_nals;

	SpeexBits spx_bits;
	void* spx_enc;
	char spx_buffer[1024];

	VideoParams rec_params;
	std::vector<VideoPacket*> video_packets;
	std::vector<AudioPacket*> audio_packets;
	std::vector<AVInfoPacket> info_packets;
	
	
#if RGB_CONVERTER == CONVERTER_SWSCALE
	struct SwsContext* sws;
#endif

	FILE* fp;
	VideoIO* io;
};

inline void VideoRecorder::setIO(VideoIO* io) {
	this->io = io;
}

inline void VideoRecorder::setFPS(int f) {
	fps = f;
}

#endif



