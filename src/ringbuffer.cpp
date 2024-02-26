// This is an incredibly simple Ring Buffer, no real magic behind it!
#include <CoreAudio/AudioServerPlugIn.h>
#include <mutex>

const int BufferSize = 11024;

class SampleRingBuffer {
    public:
        bool is_underrun = true;
        bool is_overrun = false;
        SampleRingBuffer() {}

        void push_all(Float32* buffer, UInt32 count) {
            for (UInt32 i = 0; i < count; i++) {
                push(buffer[i]);
            }
        }

        // Gets the tail distance away from the head..
        UInt32 getTailDistance() {
            // Has the head looped back round to be below the tail?
            if (head < tail) {
                return head + BufferSize - tail;
            }
            return tail - head;
        }

        void setTailDistance(UInt32 distance) {
            if (head - distance < 0)  {
                tail = head + BufferSize - distance;
            } else {
                tail = head - distance;
            }

            // We've repositioned our tail, so we currently can
            // neither be overrun, or underrun..
            is_overrun = false;
            is_underrun = false;
        }

        // Push a value to the buffer..
        void push(Float32 value) {
            buffer[head] = value;
            increment(head);

            // New value added, we can't be underrun in this scenario..
            is_underrun = false;

            if (head == tail) {
                // Buffer Overrun, flag the tail for reset when next active..
                is_overrun = true;
            }
        }

        // Pull the next value..
        Float32 pop() {

            // We're overrun, we don't want to fetch any previous data from the buffer, so reset the position.
            // Be advised, this will temporarily put us into an underrun state, but will be solved once enough
            // samples become available.
            if (is_overrun) {
                is_overrun = false;
                tail = head;
            }

            Float32 value = buffer[tail];
            increment(tail);

            // We've underrun, in this case we generally don't want to be looping back over historical data that
            // is yet to be replaced, it's up to the calling code to handle this properly until the underrun is
            // cleared by a push().
            if (tail == head) {
                // We've caught up with the head, underrun!
                is_underrun = true;
            }

            return value;
        }

    private:
        int tail = 0;
        int head = 0;

        std::mutex m_mutex;
        Float32 buffer[BufferSize];

        // Helper functions to rotate a pointer to the front / back of the buffer..
        void increment(int &i) const {
            i = (i + 1 < BufferSize) ?  (i + 1) : (0);
        }

        void decrement(int &i) const {
            i = (i - 1 >= 0) ? (i-1) : (BufferSize) - 1;
        }
};