#ifndef ROXLU_OGGMAKER_H
#define ROXLU_OGGMAKER_H

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <roxlu/Roxlu.h>

extern "C" {
#include <theora/codec.h>
#include <theora/theoraenc.h>
#include <vpx/vpx_image.h>
#include <ogg/ogg.h>
#include <libyuv.h>
#include <libswscale/swscale.h>
#include <speex/speex.h>
#include <speex/speex_header.h>
#include <vorbis/vorbisenc.h>
}

enum OGGMAKER_IN_FMT {
  OIF_RGB24,
  OIF_YUV420
};

class OggMaker {
 public:
  OggMaker();
  ~OggMaker();
  bool setup();
  void addVideoFrame(void* pixels, size_t nbytes, int last,  OGGMAKER_IN_FMT fmt = OIF_RGB24);
  void addAudioFrame(void* data, size_t nbytes); // for now 16 bit, mono, 320 frames
  void duplicateFrames(int dup);
  void print();
 private:
  std::string colorspaceToString(th_colorspace space);
  std::string pixelformatToString(th_pixel_fmt fmt);
  bool theoraSetup();
  bool vorbisSetup();
  void oggStreamFlush(ogg_stream_state& os);
  void oggStreamPageOut(ogg_stream_state& os);
 private:
  bool use_audio;
  bool use_video;
  bool is_setup;
  unsigned int image_w;
  unsigned int image_h;
  th_info ti;
  th_enc_ctx* td;
  ogg_stream_state video_oss;
  int fps;
  FILE* fp;

  th_ycbcr_buffer ycbcr;
  unsigned long yuv_w;
  unsigned long yuv_h;
  unsigned char* yuv_y;
  unsigned char* yuv_u;
  unsigned char* yuv_v;
  vpx_image_t* in_image;
  unsigned char* out_planes[3];
  int out_strides[3];
  int in_stride;
  int num_video_frames;
  int num_audio_frames;
  struct SwsContext* sws;

  rx_uint64 last_time;
  rx_uint64 millis_per_frame;
  rx_uint64 time_accum; // can be removed

  // audio
  ogg_stream_state audio_oss;
  ogg_packet vheader_comm;
  ogg_packet vheader_code;
  vorbis_info vi;
  vorbis_comment vc;
  vorbis_dsp_state vd;
  vorbis_block vb;


};


inline std::string OggMaker::colorspaceToString(th_colorspace space) {
  if(space == TH_CS_UNSPECIFIED) {
    return "TH_CS_UNSPECIFIED";
  }
  else if(space == TH_CS_ITU_REC_470M) {
    return "TH_CS_ITU_REC_470M";
  }
  else if(space == TH_CS_ITU_REC_470BG) {
    return "TH_CS_ITU_REC_470BG";
  }
  else if(space == TH_CS_NSPACES) {
    return "TH_CS_NSPACES";
  }
  else {
    return "UNKNOWN";
  }
}

inline std::string OggMaker::pixelformatToString(th_pixel_fmt fmt) {
  if(fmt == TH_PF_420) {
    return "TH_PF_420";
  }
  else if(fmt == TH_PF_RSVD) {
    return "TH_PF_RSVD";
  }
  else if(fmt == TH_PF_422) {
    return "TH_PF_422";
  }
  else if(fmt == TH_PF_444) {
    return "TH_PF_444";
  }
  else if(fmt == TH_PF_NFORMATS) {
    return "TH_PF_NFORMATS";
  }
  else {
    return "UNKNOWN";
  }
}
#endif