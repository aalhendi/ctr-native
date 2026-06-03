#ifndef PLATFORM_NATIVE_AUDIO_H
#define PLATFORM_NATIVE_AUDIO_H

int NativeAudio_PlayXATrack(int categoryID, int xaID, int volumeLeft, int volumeRight);
int NativeAudio_GetXATrackLength(int categoryID, int xaID);
int NativeAudio_IsXAPlaying(void);
int NativeAudio_GetXAMaxSample(void);
void NativeAudio_SetXAVolume(int volumeLeft, int volumeRight);
void NativeAudio_StopXA(void);
void NativeAudio_StepVBlank(void);
void NativeAudio_ClearOutputQueue(void);
#ifdef CTR_INTERNAL
void NativeAudio_GetOutputStats(int *underrunFrames, int *overflowFrames, int *queuedFrames);
#endif
int NativeAudio_GetStateSize(void);
int NativeAudio_CaptureState(void *dst, int dstSize);
int NativeAudio_RestoreState(const void *src, int srcSize);

#endif
