#ifndef recsearch_recording_filter_h
#define recsearch_recording_filter_h

class cRecording;


namespace recsearch
{
// backport from vdr 2.1.3
class cRecordingFilter {
public:
  virtual ~cRecordingFilter(void) {};
  virtual bool Filter(const cRecording *Recording) const = 0;
      ///< Returns true if the given Recording shall be displayed in the Recordings menu.
  };
}

#endif
