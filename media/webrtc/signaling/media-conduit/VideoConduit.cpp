/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "VideoConduit.h"
#include "video_engine/include/vie_errors.h"
#include "CSFLog.h"

namespace mozilla {


static const char* logTag ="WebrtcVideoSessionConduit";

const unsigned int WebrtcVideoConduit::CODEC_PLNAME_SIZE = 32;

//Factory Implementation
mozilla::RefPtr<VideoSessionConduit> VideoSessionConduit::Create()
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__); 

  WebrtcVideoConduit* obj = new WebrtcVideoConduit();

  if(!obj)
  {
    return NULL;
  }

  if(obj->Init() == kMediaConduitNoError)
  {
    CSFLogDebug(logTag, "%s Successfully Created Video Conduit", __FUNCTION__);
    return obj;
  } else {
    return NULL;
  }

}

WebrtcVideoConduit::~WebrtcVideoConduit()
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  for(unsigned i=0; i < mRecvCodecList.size(); i++)
  {
    if(mRecvCodecList[i])
    {
      delete mRecvCodecList[i];
    }
  }

   if(mCurSendCodecConfig)
   {
     delete mCurSendCodecConfig;
   }
   
  //Deal with External Capturer   
  if(mPtrViECapture)
  {
    mPtrViECapture->DisconnectCaptureDevice(mCapId);   
    mPtrViECapture->ReleaseCaptureDevice(mCapId);   
    mPtrExtCapture = NULL;    
  }

  //Deal with External Renderer   
  if(mPtrViERender)
  {
    mPtrViERender->StopRender(mChannel);   
    mPtrViERender->RemoveRenderer(mChannel);    
  }

  //Deal with the transport   
  if(mPtrViENetwork)
  {
    mPtrViENetwork->DeregisterSendTransport(mChannel);   
  }

  if(mPtrViEBase)
  {
    mPtrViEBase->StopSend(mChannel);   
    mPtrViEBase->StopReceive(mChannel);   
    mPtrViEBase->DeleteChannel(mChannel);   
  }

  if(mVideoEngine)
  {
    webrtc::VideoEngine::Delete(mVideoEngine);
  }
}

/**
 * Peforms intialization of the MANDATORY components of the Video Engine
 */
MediaConduitErrorCode WebrtcVideoConduit::Init()
{

  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  if( !(mVideoEngine = webrtc::VideoEngine::Create()) )
  {
    CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
     return kMediaConduitSessionNotInited;
  }

#if 0
  // TRACING
  mVideoEngine->SetTraceFilter(webrtc::kTraceAll);
  mVideoEngine->SetTraceFile( "Vievideotrace.out" );
#endif

  if( !(mPtrViEBase = ViEBase::GetInterface(mVideoEngine)) )
  {
    CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
     return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViECapture = ViECapture::GetInterface(mVideoEngine)) )
  {
    CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
     return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViECodec = ViECodec::GetInterface(mVideoEngine)) )
  {
    CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
     return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViENetwork = ViENetwork::GetInterface(mVideoEngine)) )
  {
    CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
     return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViERender = ViERender::GetInterface(mVideoEngine)) )
  {
    CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
     return kMediaConduitSessionNotInited;
  }
  
  
  CSFLogDebug(logTag, "%sEngine Created: Init'ng the interfaces ",__FUNCTION__); 

  if(mPtrViEBase->Init() == -1)
  {
    CSFLogError(logTag, " %s Video Engine Init Failed %d ",__FUNCTION__,
                                               mPtrViEBase->LastError());
    return kMediaConduitSessionNotInited;    
  }


  if(mPtrViEBase->CreateChannel(mChannel) == -1)
  {
    CSFLogError(logTag, " %s Channel creation Failed %d ",__FUNCTION__,
                                               mPtrViEBase->LastError());
    return kMediaConduitChannelError;
  }

  if(mPtrViENetwork->RegisterSendTransport(mChannel, *this) == -1 )
  {
    CSFLogError(logTag,  "%s ViENetwork Failed %d ", __FUNCTION__,
                                          mPtrViEBase->LastError());
    return kMediaConduitTransportRegistrationFail;
  }


  mPtrExtCapture = 0;

  if(mPtrViECapture->AllocateExternalCaptureDevice(mCapId,
                                                   mPtrExtCapture) == -1 ) 
  {
    CSFLogError(logTag, "%s Unable to Allocate capture module: %d ",
                               __FUNCTION__, mPtrViEBase->LastError()); 
    return kMediaConduitCaptureError;
  }

  if(mPtrViECapture->ConnectCaptureDevice(mCapId,mChannel) == -1)
  {
    CSFLogError(logTag, "%s Unable to Connect capture module: %d ",
                               __FUNCTION__,mPtrViEBase->LastError()); 
    return kMediaConduitCaptureError;
  }

  if(mPtrViERender->AddRenderer(mChannel,  
                                webrtc::kVideoI420, 
                                (webrtc::ExternalRenderer*) this) == -1)
  {
    CSFLogError(logTag, "%s Failed to added external renderer ", __FUNCTION__);
    return kMediaConduitInvalidRenderer;
  }


  CSFLogError(logTag, "Initialization Done");  
  
  return kMediaConduitNoError;
}


MediaConduitErrorCode 
  WebrtcVideoConduit::AttachRenderer(mozilla::RefPtr<VideoRenderer> aVideoRenderer)
{
  
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  
  //null renderer
  if(!aVideoRenderer)
  {
    CSFLogError(logTag, "%s NULL Renderer", __FUNCTION__);
    return kMediaConduitInvalidRenderer;
  }

  //Assign the new renderer - overwrites if there is already one
  mRenderer = aVideoRenderer;

  //Start Rendering if we haven't already
  if(!mEngineRendererStarted)
  {
    if(mPtrViERender->StartRender(mChannel) == -1)
    {
      CSFLogError(logTag, "%s Starting the Renderer Failed %d ", __FUNCTION__,
                                                      mPtrViEBase->LastError());
      mRenderer = NULL;
      return kMediaConduitRendererFail;
    }
    mEngineRendererStarted = true;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode 
  WebrtcVideoConduit::AttachTransport(mozilla::RefPtr<TransportInterface> aTransport)
{
 
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
 
  if(!aTransport)
  {
    CSFLogError(logTag, "%s NULL Transport ", __FUNCTION__);
    return kMediaConduitInvalidTransport;
  }

  //Assign the transport
  mTransport = aTransport;
  return kMediaConduitNoError;

}

/**
 * Note: Setting the send-codec on the Video Engine will restart the encoder,
 * sets up new SSRC and reset RTP_RTCP module with the new codec setting.
 */
MediaConduitErrorCode 
 WebrtcVideoConduit::ConfigureSendMediaCodec(const VideoCodecConfig* codecConfig)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  int error = 0;  //engine error
  bool codecFound = false; 
  webrtc::VideoCodec  video_codec;
  std::string payloadName;

  //validate basic params
  if(!codecConfig || (codecConfig->mName.length() == 0) ||
                     (codecConfig->mName.length() > CODEC_PLNAME_SIZE))
  {
    CSFLogError(logTag, "%s Invalid CodecConfig Param ", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  //Check if we have same codec already applied
  if(CheckCodecsForMatch(mCurSendCodecConfig, codecConfig))
  {
    CSFLogDebug(logTag,  "%s Codec has been applied already ", __FUNCTION__);
    return kMediaConduitNoError;
  }

    
  //transmitting already ?
  if(mEngineTransmitting)
  {
    CSFLogDebug(logTag, "%s Engine Already Sending. Attemping to Stop ", __FUNCTION__);
    
    if(mPtrViEBase->StopSend(mChannel) == -1)
    {
      error = mPtrViEBase->LastError();
      if(error == kViEBaseNotSending)
      {
        CSFLogDebug(logTag, "%s StopSend() Success ", __FUNCTION__);
        mEngineTransmitting = false;
      } else {
        CSFLogError(logTag, "%s StopSend() Failed %d ", __FUNCTION__,
                                                    mPtrViEBase->LastError());
        return kMediaConduitUnknownError;
      }
    } 
  }

  //reset the flag
  mEngineTransmitting = false;
  
  // we should be good here to set the new codec.

  memset(&video_codec, 0, sizeof(webrtc::VideoCodec));

  for(int idx=0; idx < mPtrViECodec->NumberOfCodecs(); idx++)
  {   
    if(0 == mPtrViECodec->GetCodec(idx, video_codec))
    {
      payloadName = video_codec.plName;
      if(codecConfig->mName.compare(payloadName) == 0)
      {
        CodecConfigToWebRTCCodec(codecConfig, video_codec);
        codecFound = true;
        break;
      }
    }
  }//for
  
  if(codecFound == false)
  {
    CSFLogError(logTag, "%s Codec Mismatch ", __FUNCTION__);
    return kMediaConduitInvalidSendCodec;
  }

  if(mPtrViECodec->SetSendCodec(mChannel, video_codec) == -1)
  {
     error = mPtrViEBase->LastError();
     if(error == kViECodecInvalidCodec)           
     {
        CSFLogError(logTag, "%s Invalid Send Codec", __FUNCTION__);
        return kMediaConduitInvalidSendCodec;

     } else if(error == kViECodecInUse)
     {
        CSFLogError(logTag, "%s Send Codec in Use", __FUNCTION__);
        return kMediaConduitCodecInUse;
     } 
    CSFLogError(logTag, "%s SetSendCodec Failed %d ", __FUNCTION__,
                                         mPtrViEBase->LastError());
    return kMediaConduitUnknownError;
     
  }

  if(mPtrViEBase->StartSend(mChannel) == -1)
  {
    error = mPtrViEBase->LastError();
    CSFLogError(logTag, "%s Start Send  Error %d ", __FUNCTION__, error);
    return kMediaConduitUnknownError;
  }

  //Copy the applied codec for future reference
  if(mCurSendCodecConfig)
  {
    delete mCurSendCodecConfig;
  } 
  mCurSendCodecConfig = new VideoCodecConfig(codecConfig->mType,
                                              codecConfig->mName,
                                              codecConfig->mWidth,
                                              codecConfig->mHeight);
  

  // by now we should be successfully started the transmission
  mEngineTransmitting = true;
  return kMediaConduitNoError;
}


MediaConduitErrorCode 
WebrtcVideoConduit::ConfigureRecvMediaCodecs(
                    const std::vector<VideoCodecConfig* >& codecConfigList)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  int error = 0;
  bool success = false;
  std::string  payloadName;
  
  if(!codecConfigList.size())
  {
    CSFLogError(logTag, "%s Zero number of codecs to configure", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }
  
  //Try Applying the codecs in the list
  // we treat as success if atleast one codec was applied and reception was
  // started successfully.
  for(unsigned int i=0 ; i < codecConfigList.size() && codecConfigList[i]; i++)
  { 
    if( (codecConfigList[i]->mName.length() == 0) ||
        (codecConfigList[i]->mName.length() >= CODEC_PLNAME_SIZE))
    {
      CSFLogError(logTag, "%s Invalid Payload Name Length ", __FUNCTION__);
      continue;
    }

    if(CheckCodecForMatch(codecConfigList[i]))
    {
      CSFLogDebug(logTag,  "%s Codec has been applied already ", __FUNCTION__);
      continue;
    }
   
    if(mEngineReceiving)
    {
      CSFLogDebug(logTag, "%s Engine Already Receiving . Attemping to Stop ", __FUNCTION__); 
      if(mPtrViEBase->StopReceive(mChannel) == -1)
      {
        error = mPtrViEBase->LastError();
        if(error == kViEBaseUnknownError)
        {
          CSFLogDebug(logTag, "%s StopReceive() Success ", __FUNCTION__);
          mEngineReceiving = false;
        } else {
          CSFLogError(logTag, "%s StopReceive() Failed %d ", __FUNCTION__,
                                                mPtrViEBase->LastError());
          return kMediaConduitUnknownError;
       }
      }   
    }

    //reset the flag
    webrtc::VideoCodec  video_codec;
    mEngineReceiving = false;
    memset(&video_codec, 0, sizeof(webrtc::VideoCodec));
    //Retrieve pre-populated codec structure for our codec.
    for(int idx=0; idx < mPtrViECodec->NumberOfCodecs(); idx++)
    {
      if(mPtrViECodec->GetCodec(idx, video_codec) == 0)
      {
        payloadName = video_codec.plName;
        if(codecConfigList[i]->mName.compare(payloadName) == 0)
        {
          CodecConfigToWebRTCCodec(codecConfigList[i], video_codec);
          if(mPtrViECodec->SetReceiveCodec(mChannel,video_codec) == -1)
          {
            CSFLogError(logTag, "%s Invalid Receive Codec %d ", __FUNCTION__,
                                                    mPtrViEBase->LastError());
          } else {
            CSFLogError(logTag, "%s Successfully Set the codec %s", __FUNCTION__,
                                              codecConfigList[i]->mName.c_str());
            if(CopyCodecToDB(codecConfigList[i]))
            {
              success = true;
            } else {
              CSFLogError(logTag,"%s Unable to updated Codec Database", __FUNCTION__);
              return kMediaConduitUnknownError;
            }
          } 
          break; //we found a match  
        }         
      }
    }//end for codeclist

  }//end for 

    
    
  if(!success)
  {
    CSFLogError(logTag, "%s Setting Receive Codec Failed ", __FUNCTION__);
    return kMediaConduitInvalidReceiveCodec;
  }

  //Start Receive on the video engine
  if(mPtrViEBase->StartReceive(mChannel) == -1)
  {
    error = mPtrViEBase->LastError();
    CSFLogError(logTag, "%s Start Receive Error %d ", __FUNCTION__, error);
       
    return kMediaConduitUnknownError;
  }

  // by now we should be successfully started the reception
  mEngineReceiving = true;
  DumpCodecDB();

  return kMediaConduitNoError;
}


MediaConduitErrorCode 
  WebrtcVideoConduit::SendVideoFrame(unsigned char* video_frame,
                                      unsigned int video_frame_length,
                                      unsigned short width,
                                      unsigned short height,
                                      VideoType video_type,
                                      uint64_t capture_time)
{

  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  //check for  the parameters sanity
  if(!video_frame || video_frame_length == 0 ||
                     width == 0 || height == 0)
  {
    CSFLogError(logTag,  "%s Invalid Parameters ",__FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  if(video_type != kVideoI420)
  {
    CSFLogError(logTag,  "%s VideoType Invalid. Only 1420 Supported",__FUNCTION__);
    return kMediaConduitMalformedArgument;
  }
  //Transmission should be enabled before we insert any frames.
  if(!mEngineTransmitting)
  {
    CSFLogError(logTag, "%s Engine not transmitting ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }
  
  //insert the frame to video engine in I420 format only
  if(mPtrExtCapture->IncomingFrame(video_frame, 
                                   video_frame_length,
                                   width, height,
                                   webrtc::kVideoI420,
                                   (unsigned long long)capture_time) == -1)
  {
    CSFLogError(logTag,  "%s IncomingFrame Failed %d ", __FUNCTION__, 
                                            mPtrViEBase->LastError());   
    return kMediaConduitCaptureError;
  }

  CSFLogError(logTag, "%s Inserted A Frame", __FUNCTION__);
  return kMediaConduitNoError;
}

// Transport Layer Callbacks 
MediaConduitErrorCode WebrtcVideoConduit::ReceivedRTPPacket(const void *data, int len)
{
  CSFLogError(logTag, "%s: Channel %d, Len %d ", __FUNCTION__, mChannel, len);
 
  // Media Engine should be receiving already.
  if(mEngineReceiving)
  {
    // let the engine know of a RTP packet to decode
    if(mPtrViENetwork->ReceivedRTPPacket(mChannel,data,len) == -1)
    {
      int error = mPtrViEBase->LastError();
      CSFLogError(logTag, "%s RTP Processing Failed %d ", __FUNCTION__, error);
      if(error >= kViERtpRtcpInvalidChannelId && error <= kViERtpRtcpRtcpDisabled)
      {
        return kMediaConduitRTPProcessingFailed;
      }
        return kMediaConduitRTPRTCPModuleError;
    }
  } else {
    CSFLogError(logTag, "%s Engine Error: Not Receiving !!! ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode WebrtcVideoConduit::ReceivedRTCPPacket(const void *data, int len)
{
  CSFLogError(logTag, " %s Channel %d, Len %d ", __FUNCTION__, mChannel, len);

  //Media Engine should be receiving already
  if(mEngineReceiving)
  {
    // let the engine know of RTCP packet to decode.
    if(mPtrViENetwork->ReceivedRTCPPacket(mChannel,data,len) == -1)
    {
      int error = mPtrViEBase->LastError();
      CSFLogError(logTag, "%s RTP Processing Failed %d", __FUNCTION__, error);
      if(error >= kViERtpRtcpInvalidChannelId && error <= kViERtpRtcpRtcpDisabled)
      {
        return kMediaConduitRTPProcessingFailed;
      }
        return kMediaConduitRTPRTCPModuleError;
    }
  } else {
    CSFLogError(logTag, "%s: Engine Error: Not Receiving", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  return kMediaConduitNoError;
}

//WebRTC::RTP Callback Implementation
int WebrtcVideoConduit::SendPacket(int channel, const void* data, int len)
{
  CSFLogError(logTag, "%s Channel %d, len %d ", __FUNCTION__, channel, len);

   if(mTransport && (mTransport->SendRtpPacket(data, len) == NS_OK))
   {
      CSFLogDebug(logTag, "%s Sent RTP Packet ", __FUNCTION__);
      return 0;
   } else {
      CSFLogError(logTag, "%s  Failed", __FUNCTION__);
      return -1;
   }
   
}

int WebrtcVideoConduit::SendRTCPPacket(int channel, const void* data, int len)
{
  CSFLogError(logTag,  "%s : channel %d , len %d ", __FUNCTION__, channel,len);
  
  if(mTransport && (mTransport->SendRtcpPacket(data, len) == NS_OK))
   {
      CSFLogDebug(logTag, "%s Sent RTCP Packet ", __FUNCTION__);
      return 0;
   } else {
      CSFLogError(logTag, "%s Failed", __FUNCTION__);
      return -1;
   }

  
}

// WebRTC::ExternalMedia Implementation
int 
 WebrtcVideoConduit::FrameSizeChange(unsigned int width, 
                                      unsigned int height,
                                      unsigned int numStreams)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  if(mRenderer) 
  {
    mRenderer->FrameSizeChange(width, height, numStreams);
    return 0;
  }  

  CSFLogError(logTag,  "%s Renderer is NULL ", __FUNCTION__);
  return -1;
}

int 
 WebrtcVideoConduit::DeliverFrame(unsigned char* buffer, 
                                    int buffer_size,
                                    uint32_t time_stamp, 
                                    int64_t render_time)
{
  CSFLogDebug(logTag,  "%s Buffer Size %d", __FUNCTION__, buffer_size);

  if(mRenderer)
  {
    mRenderer->RenderVideoFrame(buffer, buffer_size, time_stamp, render_time);
    return 0;
  } 

  CSFLogError(logTag,  "%s Renderer is NULL  ", __FUNCTION__);
  return -1;
}

/**
 * Conduit Uitility Functions
 */


/**
 * Copy the codec passed into Conduit's database
 */

void 
 WebrtcVideoConduit::CodecConfigToWebRTCCodec(const VideoCodecConfig* codecInfo,
                                              webrtc::VideoCodec& cinst)
 {
  
  cinst.plType  = codecInfo->mType;
  cinst.width   = codecInfo->mWidth;
  cinst.height  = codecInfo->mHeight; 
 }

bool 
 WebrtcVideoConduit::CopyCodecToDB(const VideoCodecConfig* codecInfo)
{

  if(!codecInfo)
  {
    return false;
  }
  
  VideoCodecConfig* cdcConfig = new VideoCodecConfig(codecInfo->mType,
                                                     codecInfo->mName,
                                                     codecInfo->mWidth,
                                                     codecInfo->mHeight);
 if(cdcConfig)
 {
    mRecvCodecList.push_back(cdcConfig);
    return true;
 } else
 {
    CSFLogDebug(logTag, "%s Mem Alloc Failure for Codec", __FUNCTION__);
    return false;
 }

}

/**
 * Checks if the codec is already in Conduit's database
 */
bool 
 WebrtcVideoConduit::CheckCodecForMatch(const VideoCodecConfig* codecInfo) const
{
  //local codec db is empty
  if(mRecvCodecList.size() == 0)
  {
    return false;
  }

  if(!codecInfo)
  {
    return false;
  }
  
  //the db should have atleast one codec
  for(unsigned int i=0 ; i < mRecvCodecList.size(); i++)
  {
    if(CheckCodecsForMatch(
               const_cast<VideoCodecConfig*>(mRecvCodecList[i]),codecInfo))
    {
      //match
      return true;
    }
    continue;  
  }
  //no match
  return false;
}

bool 
 WebrtcVideoConduit::CheckCodecsForMatch(const VideoCodecConfig* curCodecConfig,
                                         const VideoCodecConfig* codecInfo) const
{
  if(!curCodecConfig || !codecInfo)
  { 
    return false;
  } 

  if(curCodecConfig->mType   == codecInfo->mType &&
      (curCodecConfig->mName.compare(codecInfo->mName) == 0) &&
      curCodecConfig->mWidth  == codecInfo->mWidth &&
      curCodecConfig->mHeight == codecInfo->mHeight)
  {
    return true;
  }

 return false;
}

void
 WebrtcVideoConduit::DumpCodecDB() const
 {
    for(unsigned int i=0;i < mRecvCodecList.size(); i++)
    {
      CSFLogDebug(logTag,"Payload Name: %s", mRecvCodecList[i]->mName.c_str());
      CSFLogDebug(logTag,"Payload Type: %d", mRecvCodecList[i]->mType);
      CSFLogDebug(logTag,"Payload Width: %d", mRecvCodecList[i]->mWidth);
      CSFLogDebug(logTag,"Payload Height: %d", mRecvCodecList[i]->mHeight);
    }
 }

}// end namespace

