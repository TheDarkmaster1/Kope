/*
      * $Id: paex_record.c 1752 2011-09-08 03:21:55Z philburk $
      *
      * This program uses the PortAudio Portable Audio Library.
     * For more information see: http://www.portaudio.com
     * Copyright (c) 1999-2000 Ross Bencina and Phil Burk
     *
     * Permission is hereby granted, free of charge, to any person obtaining
     * a copy of this software and associated documentation files
     * (the "Software"), to deal in the Software without restriction,
     * including without limitation the rights to use, copy, modify, merge,
     * publish, distribute, sublicense, and/or sell copies of the Software,
     * and to permit persons to whom the Software is furnished to do so,
     * subject to the following conditions:
     *
     * The above copyright notice and this permission notice shall be
     * included in all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
     * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
     * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
     * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
     * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
     * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
     */
    
    /*
     * The text above constitutes the entire PortAudio license; however, 
     * the PortAudio community also makes the following non-binding requests:
     *
     * Any person wishing to distribute modifications to the Software is
     * requested to send the modifications to the original developer so that
     * they can be incorporated into the canonical version. It is also 
     * requested that these non-binding requests be included along with the 
     * license above.
     */
    
    #include <stdio.h>
    #include <stdlib.h>
    #include "portaudio.h"
    
    /* #define SAMPLE_RATE  (17932) // Test failure to open with this value. */
    #define SAMPLE_RATE  (44100)
    #define FRAMES_PER_BUFFER (512)
    #define NUM_SECONDS     (5)
    #define NUM_CHANNELS    (2)
    /* #define DITHER_FLAG     (paDitherOff) */
    #define DITHER_FLAG     (0) 
    
    /* Select sample format. */
    #if 1
    #define PA_SAMPLE_TYPE  paFloat32
    typedef float SAMPLE;
    #define SAMPLE_SILENCE  (0.0f)
    #define PRINTF_S_FORMAT "%.8f"
    #elif 1
    #define PA_SAMPLE_TYPE  paInt16
    typedef short SAMPLE;
    #define SAMPLE_SILENCE  (0)
    #define PRINTF_S_FORMAT "%d"
    #elif 0
    #define PA_SAMPLE_TYPE  paInt8
    typedef char SAMPLE;
    #define SAMPLE_SILENCE  (0)
    #define PRINTF_S_FORMAT "%d"
    #else
    #define PA_SAMPLE_TYPE  paUInt8
    typedef unsigned char SAMPLE;
    #define SAMPLE_SILENCE  (128)
    #define PRINTF_S_FORMAT "%d"
    #endif
    
    typedef struct
    {
        int          frameIndex;  /* Index into sample array. */
        int          maxFrameIndex;
        SAMPLE      *recordedSamples;
    }
    paTestData;
    
   
   
   /* This routine will be called by the PortAudio engine when audio is needed.
   ** It may be called at interrupt level on some machines so don't do anything
   ** that could mess up the system like calling malloc() or free().
   */
   static int playCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
   {
       paTestData *data = (paTestData*)userData;
       SAMPLE *rptr = &data->recordedSamples[data->frameIndex * NUM_CHANNELS];
       SAMPLE *wptr = (SAMPLE*)outputBuffer;
       unsigned int i;
       int finished;
       unsigned int framesLeft = data->maxFrameIndex - data->frameIndex;
   
       (void) inputBuffer; /* Prevent unused variable warnings. */
       (void) timeInfo;
       (void) statusFlags;
       (void) userData;
   
       if( framesLeft < framesPerBuffer ){
           /* final buffer... */
           for( i=0; i<framesLeft; i++ ){
               *wptr++ = *rptr++;  /* left */
               if( NUM_CHANNELS == 2 ) *wptr++ = *rptr++;  /* right */
           }
           for( ; i<framesPerBuffer; i++ ){
               *wptr++ = 0;  /* left */
               if( NUM_CHANNELS == 2 ) *wptr++ = 0;  /* right */
           }
           data->frameIndex += framesLeft;
           finished = paComplete;
       }else{
           for( i=0; i<framesPerBuffer; i++ ){
               *wptr++ = *rptr++;  /* left */
               if( NUM_CHANNELS == 2 ) *wptr++ = *rptr++;  /* right */
           }
           data->frameIndex += framesPerBuffer;
           finished = paContinue;
       }
       return finished;
   }
   
   /*******************************************************************/
   int main(void){
       PaStreamParameters  outputParameters;
       PaStream*           stream;
       PaError             err = paNoError;
       paTestData          data;
       int                 totalFrames;
       int                 numSamples;
       int                 numBytes;
   
   
       data.maxFrameIndex = totalFrames = NUM_SECONDS * SAMPLE_RATE; /* Record for a few seconds. */
       data.frameIndex = 0;
       numSamples = totalFrames * NUM_CHANNELS;
       numBytes = numSamples * sizeof(SAMPLE);
       data.recordedSamples = (SAMPLE *) malloc( numBytes ); /* From now on, recordedSamples is initialised. */
       if( data.recordedSamples == NULL ){
           printf("Could not allocate record array.\n");
           goto done;
       }
   
       if( err != paNoError ) goto done;
   
        FILE * pFile;
        long lSize;
        size_t result;

        pFile = fopen ( "recorded.raw" , "rb" );
        if (pFile==NULL) {fputs ("Error opening file!",stderr); exit (1);}

        // obtain file size:
        fseek (pFile , 0 , SEEK_END);
        lSize = ftell (pFile);
        rewind (pFile);

        // copy the file into the buffer:
	result = fread( data.recordedSamples, NUM_CHANNELS * sizeof(SAMPLE), lSize / (NUM_CHANNELS * sizeof(SAMPLE)), pFile );
        if (result != lSize / (NUM_CHANNELS * sizeof(SAMPLE))) {fputs ("Reading error, file may be corrupted\n",stderr); exit (3);}
        fclose (pFile);

	



       /* Playback recorded data.  -------------------------------------------- */
       data.frameIndex = 0;
   
       outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
       if (outputParameters.device == paNoDevice) {
           fprintf(stderr,"Error: No default output device.\n");
           goto done;
       }
       outputParameters.channelCount = 2;                     /* stereo output */
       outputParameters.sampleFormat =  PA_SAMPLE_TYPE;
       outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
       outputParameters.hostApiSpecificStreamInfo = NULL;
   
       printf("\n=== Now playing back. ===\n"); fflush(stdout);
       err = Pa_OpenStream(
                 &stream,
                 NULL, /* no input */
                 &outputParameters,
                 SAMPLE_RATE,
                 FRAMES_PER_BUFFER,
                 paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                 playCallback,
                 &data );
       if( err != paNoError ) goto done;
   
       if( stream )
       {
           err = Pa_StartStream( stream );
           if( err != paNoError ) goto done;
           
           printf("Waiting for playback to finish.\n"); fflush(stdout);
   
           while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) Pa_Sleep(100);
           if( err < 0 ) goto done;
           
           err = Pa_CloseStream( stream );
           if( err != paNoError ) goto done;
           
           printf("Done.\n"); fflush(stdout);
       }
   
   done:
       Pa_Terminate();
       if( data.recordedSamples )       /* Sure it is NULL or valid. */
           free( data.recordedSamples );
       if( err != paNoError ){
           fprintf( stderr, "An error occured while using the portaudio stream\n" );
           fprintf( stderr, "Error number: %d\n", err );
           fprintf( stderr, "Error message: %s\n", Pa_GetErrorText( err ) );
           err = 1;          /* Always return 0 or 1, but no other return codes. */
       }
       return err;
   }
