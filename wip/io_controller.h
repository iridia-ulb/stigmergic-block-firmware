#ifndef IO_CONTROLLER_H
#define IO_CONTROLLER_H

#define IO_BUFFER_SIZE

// template<INDEX_TYPE = uint8_t, CONTENT_TYPE = uint8_t, LENGTH = 64>
// CRingBuffer
class CRingBuffer {

public:

   INDEX_TYPE Write(const CONTENT_TYPE* pt_buffer, INDEX_TYPE t_n_bytes) {
      INDEX_TYPE tIdx, tHeadNext
      for(tIdx = 0; tIdx < t_n_bytes; tIdx++) {
         tHeadNext = (m_tHead + 1) % LENGTH;
         if(tHeadNext == m_tTail) {
            break;
         }
         else {
            m_sTxBuffer.Buffer[m_sTxBuffer.Head] = pun_buffer[unIdx]
            m_sTxBuffer.Head = unHeadNext;
         }
      }
      return unIdx;
   }

   INDEX_TYPE Read(uint8_t* pun_buffer, uint8_t un_n_bytes) {
      uint8_t unIdx;

      for(unIdx = 0; unIdx < un_n_bytes; unIdx++) {
         if(m_sRxBuffer.Head != m_sRxBuffer.Head) {
            break;
         }
         else {
            pun_buffer[unIdx] = m_sRxBuffer.Buffer[m_sRxBuffer.Tail];
            m_sRxBuffer.Tail = (m_sRxBuffer.Tail + 1) % IO_BUFFER_SIZE;
         }
      }

      return unIdx;
   }

   INDEX_TYPE Capacity() {
      return ((m_tHead + 1) % LENGTH != m_tTail);
   }

protected:

   INDEX_TYPE m_tHead;
   INDEX_TYPE m_tTail;
   CONTENT_TYPE m_ptBuffer[LENGTH];

}

#endif


