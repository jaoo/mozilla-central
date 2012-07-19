/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef FAKE_MEDIA_STREAM_H_
#define FAKE_MEDIA_STREAM_H_

#include "nsNetCID.h"
#include "nsITimer.h"
#include "nsComponentManagerUtils.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"

// #includes from MediaStream.h
#include "mozilla/Mutex.h"
#include "AudioSegment.h"
#include "MediaSegment.h"
#include "StreamBuffer.h"
#include "nsAudioStream.h"
#include "nsTArray.h"
#include "nsIRunnable.h"
#include "nsISupportsImpl.h"

namespace mozilla {
   class MediaStreamGraph;
   class MediaSegment;
};

class Fake_MediaStreamListener
{
public:
  virtual ~Fake_MediaStreamListener() {}

  virtual void NotifyQueuedTrackChanges(mozilla::MediaStreamGraph* aGraph, mozilla::TrackID aID,
                                        mozilla::TrackRate aTrackRate,
                                        mozilla::TrackTicks aTrackOffset,
                                        PRUint32 aTrackEvents,
                                        const mozilla::MediaSegment& aQueuedMedia)  = 0;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Fake_MediaStreamListener)
};


// Note: only one listener supported
class Fake_MediaStream {
public:
  Fake_MediaStream () : mListener(NULL) {}
  virtual ~Fake_MediaStream() {}

  virtual nsresult Start() { return NS_OK; }
  virtual nsresult Stop() { return NS_OK; }

  void AddListener(Fake_MediaStreamListener *aListener) {
    mListener = aListener;
  }

 protected:
  Fake_MediaStreamListener *mListener;
};

class Fake_nsDOMMediaStream
{
public:
  Fake_nsDOMMediaStream() : mMediaStream(new Fake_MediaStream()) {}
  Fake_nsDOMMediaStream(Fake_MediaStream *stream) : 
      mMediaStream(stream) {}

  virtual ~Fake_nsDOMMediaStream() {}

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(Fake_nsDOMMediaStream)

  Fake_MediaStream *GetStream() { return mMediaStream; }

  // Hints to tell the SDP generator about whether this
  // MediaStream probably has audio and/or video
  enum {
    HINT_CONTENTS_AUDIO = 0x00000001U,
    HINT_CONTENTS_VIDEO = 0x00000002U
  };
  PRUint32 GetHintContents() const { return mHintContents; }
  void SetHintContents(PRUint32 aHintContents) { mHintContents = aHintContents; }

private:
  Fake_MediaStream *mMediaStream;

  // tells the SDP generator about whether this
  // MediaStream probably has audio and/or video
  PRUint32 mHintContents;
};

class Fake_MediaStreamGraph
{
public:
  virtual ~Fake_MediaStreamGraph() {}
};


class Fake_AudioStreamSource : public Fake_MediaStream,
                               public nsITimerCallback {
 public:
  Fake_AudioStreamSource() : Fake_MediaStream() {}

  virtual nsresult Start();
  virtual nsresult Stop();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

 private:
  nsCOMPtr<nsITimer> mTimer;
};



typedef Fake_nsDOMMediaStream nsDOMMediaStream;

namespace mozilla {
typedef Fake_MediaStream MediaStream;
typedef Fake_MediaStreamListener MediaStreamListener;
}

#endif
