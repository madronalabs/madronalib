
// MadronaLib: a C++ framework for DSP applications.
// Copyright (c) 2013 Madrona Labs LLC. http://www.madronalabs.com
// Distributed under the MIT license: http://madrona-labs.mit-license.org/

#ifndef ML_PROC_RINGBUFFER_H
#define ML_PROC_RINGBUFFER_H

#include "MLProc.h"
#include "pa_ringbuffer.h"

extern const uint32_t RingBufferConstants[16];

// default size in samples.  should equal kMLSignalViewBufferSize. (MLUI.h)
// But we don't want to refer to UI code here.
const int kMLRingBufferDefaultSize = 128;

enum 
{
	eMLRingBufferNoTrash = 0,
	eMLRingBufferUpTrig = 1,
	eMLRingBufferMostRecent = 2
};

// ----------------------------------------------------------------
// class definition


class MLProcRingBuffer : public MLProc
{
public:
	MLProcRingBuffer();
	~MLProcRingBuffer();
	
	void clear(){};
	void process(const int n);		

	// read the buffer contents out to the specified row of the given signal.
	int readToSignal(MLSignal& outSig, int samples, int row=0);
	const MLSignal& getOutputSignal();
	MLProcInfoBase& procInfo() { return mInfo; }

private:
	err resize(); // rebuilds buffer 
	void doParams(void);	
	MLProcInfo<MLProcRingBuffer> mInfo;

	MLSignal mRing;
	MLSignal mTrashSignal;	
	PaUtilRingBuffer mBuf;
	
	MLSignal test;
	MLSample mTrig1;
};




/**
namespace {


 * The FrameRingBuffer class implements a ringbuffer for the communication
 * between two threads. One thread - the producer thread - may only write
 * data to the ringbuffer. The other thread - the consumer thread - may
 * only read data from the ringbuffer.
 *
 * Given that these two threads only use the appropriate functions, no
 * other synchronization is required to ensure that the data gets safely
 * from the producer thread to the consumer thread. However, all operations
 * that are provided by the ringbuffer are non-blocking, so that you may
 * need a condition or other synchronization primitive if you want the
 * producer and/or consumer to block if the ringbuffer is full/empty.
 *
 * Implementation: the synchronization between the two threads is only
 * implemented by two index variables (read_frame_pos and write_frame_pos)
 * for which atomic integer reads and writes are required. Since the
 * producer thread only modifies the write_frame_pos and the consumer thread
 * only modifies the read_frame_pos, no compare-and-swap or similar
 * operations are needed to avoid concurrent writes.



template<class T>
class FrameRingBuffer {
  //BIRNET_PRIVATE_COPY (FrameRingBuffer);
private:
  typedef typename vector<T>::iterator BufferIterator;

  vector<T>     m_buffer;
  volatile int  m_read_frame_pos;
  volatile int  m_write_frame_pos;
  guint         m_elements_per_frame;

  void write_memory_barrier()
  {
    static volatile int dummy = 0;
	//  writing this dummy integer should ensure that all prior writes
	//  are committed to memory
    Atomic::int_set (&dummy, 0x12345678);
  }
  
public:
  FrameRingBuffer (guint n_frames = 0,
                   guint elements_per_frame = 1)
  {
    resize (n_frames, elements_per_frame);
  }


//   * Checks available read space in the ringbuffer.
 //  * This function should be called from the consumer thread.
  // *
 //  * @returns the number of frames that are available for reading
//   

  guint
  get_readable_frames()
  {
    int wpos = Atomic::int_get (&m_write_frame_pos);
    int rpos = Atomic::int_get (&m_read_frame_pos);
    int size = m_buffer.size() / m_elements_per_frame;

    if (wpos < rpos)              //  wpos == rpos -> empty ringbuffer 
      wpos += size;

    return wpos - rpos;
  }
//
//   * Reads data from the ringbuffer; if there is not enough data
  // * in the ringbuffer, the function will not block.
 //  * This function should be called from the consumer thread.
//   *
 //  * @returns the number of successfully read frames
  // 
  
	
guint
  read (guint  n_frames,
        T     *frames)
  {
    int rpos = Atomic::int_get (&m_read_frame_pos);
    guint size = m_buffer.size() / m_elements_per_frame;
    guint can_read = min (get_readable_frames(), n_frames);

    BufferIterator start = m_buffer.begin() + rpos * m_elements_per_frame;
    guint read1 = min (can_read, size - rpos) * m_elements_per_frame;
    copy (start, start + read1, frames);

    guint read2 = can_read * m_elements_per_frame - read1;
    copy (m_buffer.begin(), m_buffer.begin() + read2, frames + read1);

    Atomic::int_set (&m_read_frame_pos, (rpos + can_read) % size);
    return can_read;
  }



   * Checks available write space in the ringbuffer.
   * This function should be called from the producer thread.
   *
   * @returns the number of frames that can be written


  guint
  get_writable_frames()
  {
    int wpos = Atomic::int_get (&m_write_frame_pos);
    int rpos = Atomic::int_get (&m_read_frame_pos);
    guint size = m_buffer.size() / m_elements_per_frame;

    if (rpos <= wpos)        //        wpos == rpos -> empty ringbuffer 
      rpos += size;

    // the extra element allows us to see the difference between an empty/full 
ringbuffer
    return rpos - wpos - 1;
  }



   * Writes data to the ringbuffer; if there is not enough data
   * in the ringbuffer, the function will not block.
   * This function should be called from the producer thread.
   *
   * @returns the number of successfully written frames


  guint write (guint n_frames, const T *frames)
  {
    int wpos = Atomic::int_get (&m_write_frame_pos);
    guint size = m_buffer.size() / m_elements_per_frame;
    guint can_write = min (get_writable_frames(), n_frames);

    BufferIterator start = m_buffer.begin() + wpos * m_elements_per_frame;
    guint write1 = min (can_write, size - wpos) * m_elements_per_frame;
    copy (frames, frames + write1, start);

    guint write2 = can_write * m_elements_per_frame - write1;
    copy (frames + write1, frames + write1 + write2, m_buffer.begin());
 
    // It is important that the data from the previous writes get committed
    // to memory *before* the index variable is updated. Otherwise, the
    // consumer thread could be reading invalid data, if the index variable
    // got written before the rest of the data (when unordered writes are
    // performed).
    write_memory_barrier();

    Atomic::int_set (&m_write_frame_pos, (wpos + can_write) % size);
    return can_write;
  }


   * Returns the maximum number of frames that the ringbuffer can contain.
   *
   * This function can be called from any thread.


  guint
  size() const
  {
    // the extra element allows us to see the difference between an empty/full 
ringbuffer
    return (m_buffer.size() - 1) / m_elements_per_frame;
  }



   * Clears the ringbuffer.
   *
   * This function is @emph{not} threadsafe, and can not be used while
   * either the producer thread or the consumer thread are modifying
   * the ringbuffer.


  void
  clear()
  {
    Atomic::int_set (&m_read_frame_pos, 0);
    Atomic::int_set (&m_write_frame_pos, 0);
  }


   * Resizes and clears the ringbuffer.
   *
   * This function is @emph{not} threadsafe, and can not be used while
   * either the producer thread or the consumer thread are modifying
   * the ringbuffer.


  void
  resize (guint n_frames,
          guint elements_per_frame = 1)
  {
    m_elements_per_frame = elements_per_frame;
    // the extra element allows us to see the difference between an empty/full 
ringbuffer
    m_buffer.resize ((n_frames + 1) * m_elements_per_frame);
    clear();
  }
};


*/

#endif // ML_PROC_RINGBUFFER_H
