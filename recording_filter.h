#ifndef recsearch_recording_filter_h
#define recsearch_recording_filter_h

class cRecording;


namespace recsearch
{
class cRecordingFilter {
public:
  cRecordingFilter(void) {};
  virtual ~cRecordingFilter(void) {};
  virtual bool Filter(const cRecording *Recording) = 0;
  };
}

#endif
