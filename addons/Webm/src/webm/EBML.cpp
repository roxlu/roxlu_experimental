#include <webm/EBML.h>

EBML::EBML() 
  :cb_peek(NULL)
  ,cb_read(NULL)
  ,cb_skip(NULL)
  ,cb_user(NULL)
  ,cb_write(NULL)
  ,cb_close(NULL)
  ,time_cluster_started(0)
{

}

EBML::~EBML() {
  close();
}

void EBML::setCallbacks(ebml_write_cb writeCB,
                        ebml_close_cb closeCB,
                        ebml_peek_cb peekCB, 
                        ebml_read_cb readCB, 
                        ebml_skip_cb skipCB,
                        void* user) 
{
  cb_write = writeCB;
  cb_close = closeCB;
  cb_peek = peekCB;
  cb_read = readCB;
  cb_skip = skipCB;
  cb_user = user;
}

int EBML::open(EBMLHeader header) {
  
  writeID(ID_EBML, &buffer);

  {
    eu8(ID_EBML_VERSION, header.ebml_version, &tmp_buffer);
    eu8(ID_EBML_READ_VERSION, header.ebml_read_version, &tmp_buffer);
    eu8(ID_EBML_MAX_ID_LENGTH, header.ebml_max_id_length, &tmp_buffer);
    eu8(ID_EBML_MAX_SIZE_LENGTH, header.ebml_max_size_length, &tmp_buffer);
    estr(ID_DOCTYPE, header.doctype, &tmp_buffer);
    eu8(ID_DOCTYPE_VERSION, header.doctype_version, &tmp_buffer);
    eu8(ID_DOCTYPE_READ_VERSION, header.doctype_read_version, &tmp_buffer);
  }  

  writeDataSize(tmp_buffer.size(), &buffer);

  flush(&buffer);
  flush(&tmp_buffer);
  return 1;
}

int EBML::openSegment(EBMLSegmentInfo info) {
  assert(tmp_buffer.size() == 0);

  writeID(ID_SEGMENT, &buffer);
  writeDataSizeUnknown(&buffer);
  flush(&buffer);

  writeID(ID_INFO, &buffer);
  {
    estr(ID_TITLE, info.title, &tmp_buffer);
    eunumber(ID_TIMECODE_SCALE, info.timecode_scale, &tmp_buffer);
    estr(ID_MUXING_APP, info.muxing_app, &tmp_buffer);
    estr(ID_WRITING_APP, info.writing_app, &tmp_buffer);
  }

  writeDataSize(tmp_buffer.size(), &buffer);

  flush(&buffer);
  flush(&tmp_buffer);

  return 1;
}

int EBML::openTracks(std::vector<EBMLTrack> tracks) {
  assert(tmp_buffer.size() == 0);
  assert(buffer.size() == 0); 

  writeID(ID_TRACKS, &buffer);
  
  {
    std::vector<char> track_buffer;
    std::vector<char> spec_buffer;

    for(std::vector<EBMLTrack>::iterator it = tracks.begin(); it != tracks.end(); ++it) {
      EBMLTrack& tr = *it;
      eu8(ID_TRACK_NUMBER, tr.number, &track_buffer);
      eunumber(ID_TRACK_UID, tr.uid, &track_buffer);
      estr(ID_CODEC_ID, tr.codec_id, &track_buffer);
      eu8(ID_TRACK_TYPE, tr.type, &track_buffer);
      
      if(tr.type == EBML_TRACK_TYPE_VIDEO) {
        eu16(ID_PIXEL_WIDTH, tr.vid_pix_width, &spec_buffer);
        eu16(ID_PIXEL_HEIGHT, tr.vid_pix_height, &spec_buffer);
      }
      else if(tr.type == EBML_TRACK_TYPE_AUDIO) {
        RX_ERROR(("@todo we need to encode the audio specs"));
      }

      writeID(ID_VIDEO, &track_buffer);
      writeDataSize(spec_buffer.size(), &track_buffer);
      write(&spec_buffer[0], spec_buffer.size(),  &track_buffer);

      writeID(ID_TRACK_ENTRY, &tmp_buffer);
      writeDataSize(track_buffer.size(), &tmp_buffer);
      write(&track_buffer[0], track_buffer.size(), &tmp_buffer);

      track_buffer.clear();
      spec_buffer.clear();
    }
  }

  writeDataSize(tmp_buffer.size(), &buffer);

  flush(&buffer);
  flush(&tmp_buffer);

  return 1;
}

int EBML::openCluster(EBMLCluster c) {
  assert(buffer.size() == 0);

  writeID(ID_CLUSTER, &buffer);
  writeDataSizeUnknown(&buffer);

  eunumber(ID_TIMECODE, c.timecode, &buffer);

  flush(&buffer);

  return 1;
}

int EBML::addSimpleBlock(EBMLSimpleBlock b) {
  assert(buffer.size() == 0);
  assert(tmp_buffer.size() == 0);

  if(!time_cluster_started) {
    RX_WARNING(("Create new cluster"));
    time_cluster_started = b.timestamp;
    
    EBMLCluster c;
    c.timecode = time_cluster_started;
    openCluster(c);
  }

  /* create a new cluser every X-millis */
  short int timecode = (b.timestamp - time_cluster_started);
  if(timecode > 5000) {
    RX_ERROR(("we need to create a new cluster"));
    time_cluster_started = b.timestamp;
    timecode = 0;
    
    EBMLCluster c;
    c.timecode = time_cluster_started;
    openCluster(c);
  }

  RX_VERBOSE(("Add EBMLSimpleBlock: %lld, time_cluster_started: %lld, relative timecode: %lld ns, %d ms", b.timestamp, time_cluster_started, b.timestamp - time_cluster_started,timecode) );
  
  writeDataSize(b.track_number, &tmp_buffer);
  wu16(timecode, &tmp_buffer);
  wu8(b.flags, &tmp_buffer);
  write(b.data, b.nbytes, &tmp_buffer);
  
  writeID(ID_SIMPLE_BLOCK, &buffer);
  RX_VERBOSE(("SIMPLE BLOCK SIZE: %zu", tmp_buffer.size()));
  writeDataSize(tmp_buffer.size(), &buffer);
  flush(&buffer);
  flush(&tmp_buffer);
  
  return 1;
}

void EBML::printElement(uint64_t id, 
                        uint64_t dataSize, 
                        int dataType, 
                        char* buffer, 
                        size_t bufferSize)
{

  printf("%s  ", idToString(id).c_str()); 
  if(dataSize == EBML_DATA_SIZE_UNKNOWN) {
    printf("( EBML_DATA_SIZE_UNKNOWN ) - ");
  }
  else {
    printf("( %llu ) - ", dataSize);
  }

  if(dataType == EBML_SKIP) {
    printf(" _SKIPPING %llu BYTES_ ", dataSize);
    cb_skip(dataSize, cb_user);
  }
  else if(dataSize < bufferSize && dataType != EBML_NONE) { 
    size_t bytes_read = readData(buffer, dataSize);
    if(dataType == EBML_CHAR ) {
      for(size_t i = 0; i < bytes_read; ++i) {
        printf("%c", (char)buffer[i]);
      }
    }
    else if(dataType == EBML_INT) {
      for(size_t i = 0; i < bytes_read; ++i) {
        printf("%d", (unsigned char)buffer[i]);
      }
    }
    else if(dataType == EBML_HEX) {
      for(size_t i = 0; i < bytes_read; ++i) {
        printf("%02X ", (unsigned char)buffer[i]);
      }
    }
  }
  printf("\n");
}


// @todo cleanup
void EBML::parse() {
  char buffer[EBML_TMP_BUFFER_SIZE]; 
  bool must_parse = true;
  int count = 0;

  uint64_t id = readID();
  uint64_t size = readDataSize();
  /*
  unsigned char* p = (unsigned char*)&size;
  for(int i = 0; i < 8; ++i) {
    printf("%02X ", p[i]);
  }
  printf(" <---\n");
  */

  unsigned char* id_ptr = (unsigned char*)&id;
  switch(id) {
    case ID_EBML:                     { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_EBML_VERSION:             { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_EBML_READ_VERSION:        { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_EBML_MAX_ID_LENGTH:       { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_EBML_MAX_SIZE_LENGTH:     { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_DOCTYPE:                  { printElement(id, size, EBML_CHAR, buffer);    return parse(); }
    case ID_DOCTYPE_VERSION:          { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_DOCTYPE_READ_VERSION:     { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_VOID:                     { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_SEGMENT:                  { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_SEGMENT_UID:              { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_TITLE:                    { printElement(id, size, EBML_CHAR, buffer);    return parse(); }
    case ID_SEEK_HEAD:                { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_SEEK:                     { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_SEEK_ID:                  { printElement(id, size, EBML_INT, buffer);     return parse(); }
    case ID_SEEK_POSITION:            { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_INFO:                     { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_TIMECODE_SCALE:           { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_DURATION:                 { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_DATE_UTC:                 { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_MUXING_APP:               { printElement(id, size, EBML_CHAR, buffer);    return parse(); }
    case ID_WRITING_APP:              { printElement(id, size, EBML_CHAR, buffer);    return parse(); }
    case ID_TRACKS:                   { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_TRACK_ENTRY:              { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_TRACK_NUMBER:             { printElement(id, size, EBML_INT, buffer);     return parse(); }
    case ID_TRACK_UID:                { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_TRACK_TYPE:               { printElement(id, size, EBML_INT, buffer);     return parse(); }
    case ID_TRACK_DEFAULT_DURATION:   { printElement(id, size, EBML_INT, buffer);     return parse(); }
    case ID_TRACK_TIMECODE_SCALE:     { printElement(id, size, EBML_INT, buffer);     return parse(); }
    case ID_CODEC_ID:                 { printElement(id, size, EBML_CHAR, buffer);    return parse(); }
    case ID_CODEC_PRIVATE:            { printElement(id, size, EBML_SKIP, buffer);    return parse(); }
    case ID_CODEC_NAME:               { printElement(id, size, EBML_CHAR, buffer);    return parse(); }
    case ID_VIDEO:                    { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_PIXEL_WIDTH:              { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_PIXEL_HEIGHT:             { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_AUDIO:                    { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_SAMPLING_FREQUENCY:       { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_CHANNELS:                 { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_BIT_DEPTH:                { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_CUES:                     { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_CUE_POINT:                { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_CUE_TIME:                 { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_CUE_TRACK_POSITIONS:      { printElement(id, size, EBML_HEX, buffer);     return parse(); }
    case ID_CLUSTER:                  { printElement(id, size, EBML_NONE, buffer);    return parse(); }
    case ID_TIMECODE:                 { printElement(id, size, EBML_HEX, buffer);     return parse(); }

    case ID_SIMPLE_BLOCK: {
      // @todo, we assume track size is always stored in one byte
      printf("ID_SIMPLE_BLOCK, size: %llu\n", size);
      uint64_t bytes_to_skip = size;
      uint64_t track = readDataSize();
      bytes_to_skip -= 1; 
      int16_t timecode = ru16();
      bytes_to_skip -= 2;
      uint8_t flags = ru8();
      bytes_to_skip -= 1;
      cb_skip(bytes_to_skip, cb_user);
      printf(" - TRACK: %llu\n", track);
      printf(" - TIMECODE: %d\n", timecode);
      printf(" - FLAGS: %02X\n", flags);
      printf(" - DATA SIZE: : %llu\n", bytes_to_skip);
      return parse();
    }
    default: {
      printf("Unhandled id: ");
      for(int i = 0; i < 8; ++i) {
        printf("%02X ", id_ptr[7-i]);
      }
      printf("\n");
      return;
    };
  };
}



std::string EBML::idToString(uint64_t id) {
  switch(id) {
    case ID_EBML:                     { return "ID_EBML";                                   }
    case ID_EBML_VERSION:             { return "ID_EBML_VERSION";                           }
    case ID_EBML_READ_VERSION:        { return "ID_EBML_READ_VERSION";                      } 
    case ID_EBML_MAX_ID_LENGTH:       { return "ID_EBML_MAX_ID_LENGTH";                     }
    case ID_EBML_MAX_SIZE_LENGTH:     { return "ID_EBML_MAX_SIZE_LENGTH";                   }
    case ID_DOCTYPE:                  { return "ID_DOCTYPE";                                } 
    case ID_DOCTYPE_VERSION:          { return "ID_DOCTYPE_VERSION";                        }
    case ID_DOCTYPE_READ_VERSION:     { return "ID_DOCTYPE_READ_VERSION";                   }  
    case ID_VOID:                     { return "ID_VOID";                                   } 
    case ID_SEGMENT:                  { return "ID_SEGMENT";                                }
    case ID_SEGMENT_UID:              { return "ID_SEGMENT_UID";                            }  
    case ID_TITLE:                    { return "ID_TITLE";                                  } 
    case ID_SEEK_HEAD:                { return "ID_SEEK_HEAD";                              }
    case ID_SEEK:                     { return "ID_SEEK";                                   } 
    case ID_SEEK_ID:                  { return "ID_SEEK_ID";                                } 
    case ID_SEEK_POSITION:            { return "ID_SEEK_POSITION";                          }
    case ID_INFO:                     { return "ID_INFO";                                   } 
    case ID_TIMECODE_SCALE:           { return "ID_TIMECODE_SCALE" ;                        } 
    case ID_DURATION:                 { return "ID_DURATION";                               }
    case ID_DATE_UTC:                 { return "ID_DATE_UTC";                               } 
    case ID_MUXING_APP:               { return "ID_MUXING_APP";                             }
    case ID_WRITING_APP:              { return "ID_WRITING_APP";                            } 
    case ID_TRACKS:                   { return "ID_TRACKS";                                 }
    case ID_TRACK_ENTRY:              { return "ID_TRACK_ENTRY";                            } 
    case ID_TRACK_NUMBER:             { return "ID_TRACK_NUMBER";                           } 
    case ID_TRACK_UID:                { return "ID_TRACK_UID";                              } 
    case ID_TRACK_TYPE:               { return "ID_TRACK_TYPE";                             } 
    case ID_TRACK_DEFAULT_DURATION:   { return "ID_TRACK_DEFAULT_DURATION";                 } 
    case ID_TRACK_TIMECODE_SCALE:     { return "ID_TRACK_TIMECODE_SCALE";                   } 
    case ID_CODEC_ID:                 { return "ID_CODEC_ID";                               } 
    case ID_CODEC_PRIVATE:            { return "ID_CODEC_PRIVATE";                          } 
    case ID_CODEC_NAME:               { return "ID_CODEC_NAME";                             } 
    case ID_VIDEO:                    { return "ID_VIDEO";                                  }
    case ID_PIXEL_WIDTH:              { return "ID_PIXEL_WIDTH";                            } 
    case ID_PIXEL_HEIGHT:             { return "ID_PIXEL_HEIGHT";                           } 
    case ID_AUDIO:                    { return "ID_AUDIO";                                  } 
    case ID_SAMPLING_FREQUENCY:       { return "ID_SAMPLING_FREQUENCY";                     } 
    case ID_CHANNELS:                 { return "ID_CHANNELS";                               } 
    case ID_BIT_DEPTH:                { return "ID_BIT_DEPTH";                              } 
    case ID_CUES:                     { return "ID_CUES";                                   } 
    case ID_CUE_POINT:                { return "ID_CUE_POINT";                              } 
    case ID_CUE_TIME:                 { return "ID_CUE_TIME";                               } 
    case ID_CUE_TRACK_POSITIONS:      { return "ID_CUE_TRACK_POSITIONS";                    } 
    case ID_CLUSTER:                  { return "ID_CLUSTER";                                } 
    case ID_TIMECODE:                 { return "ID_TIMECODE";                               } 
    case ID_SIMPLE_BLOCK:             { return "ID_SIMPLE_BLOCK";                           } 
    default:                          { return "UNKNOWN";                                   } 
  };
}

EBMLFile::EBMLFile() 
  :fp(NULL)
{
}

EBMLFile::~EBMLFile() {
  if(fp) {
    fclose(fp);
    fp = NULL;
  }
}

bool EBMLFile::open(const std::string filepath) {
  fp = fopen(filepath.c_str(), "wb+");
  if(!fp) {
    RX_ERROR(("Cannot open: %s", filepath.c_str()));
    return false;
  }
  return true;
}

size_t EBMLFile::write(char* data, size_t nbytes, void* user) {
  EBMLFile* w = static_cast<EBMLFile*>(user);
  int r = fwrite(data, nbytes, 1, w->fp);
  if(r != 1) {
    RX_ERROR(("Cannot write data to ebml file."));
    return 0;
  }
  return nbytes;
}

void EBMLFile::close(void* user) {
  EBMLFile* w = static_cast<EBMLFile*>(user);
  if(w->fp == NULL) {
    RX_WARNING(("already closed."));
    return;
  }

  fclose(w->fp);
  w->fp = NULL;
  RX_VERBOSE(("closed file."));
}

size_t EBMLFile::peek(char* dest, void* user) {
  EBMLFile* w = static_cast<EBMLFile*>(user);
  *dest = fgetc(w->fp);
  ungetc(*dest, w->fp);
  return 1;
}


size_t EBMLFile::read(char* dest, size_t nbytes, void* user) {
  EBMLFile* w = static_cast<EBMLFile*>(user);
  int r = fread(dest, nbytes, 1, w->fp);
  if(r != 1) {
    RX_ERROR(("cannot read from file."));
    return 0;
  }
  return nbytes;
}

size_t EBMLFile::skip(size_t nbytes, void* user) {
  EBMLFile* w = static_cast<EBMLFile*>(user);
  if(fseek(w->fp, nbytes, SEEK_CUR) != 0) {
    RX_ERROR(("cannot fseek()."));
    return 0;
  }
  return nbytes;
}
