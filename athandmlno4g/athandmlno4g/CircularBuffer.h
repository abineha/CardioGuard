#pragma once

template<typename T, int bufferSize>
class CircularBuffer {
private:
  T buffer[bufferSize];   // Fixed-size array to store elements.
  int head;  //double ended q, Index of the next element to insert.
  int tail;  // Index of the next element to read/remove.
  bool isFull;  // Flag to indicate whether the buffer is full.

public:
  CircularBuffer() : head(0), tail(0), isFull(false) {
    // Initialize the buffer
    for (int i = 0; i < bufferSize; ++i) {
        buffer[i] = T();
    }
  }

  void push(T value) {
    buffer[head] = value;
    head = (head + 1) % bufferSize;
    if (isFull) {
        tail = (tail + 1) % bufferSize;
    }
    isFull = head == tail;
  }

  bool isEmpty() const {
     return (!isFull && (head == tail));
  }

  bool isFilled() const {
     return isFull;
  }

  int size() const {
    if (isFull) {
        return bufferSize;
    }
    if (head >= tail) {
        return head - tail;
    }
    return bufferSize + head - tail;
  }

  void clear() {
    head = tail = 0;
    isFull = false;
    for (int i = 0; i < bufferSize; ++i) {
        buffer[i] = T();
    }
  }

  T& operator[](int index) {
    return buffer[(tail + index) % bufferSize];
  }

  const T& operator[](int index) const {
    return buffer[(tail + index) % bufferSize];
  }
};