#include "SeeedmmWave.h"

/**
 * @brief Print the buffer content in hexadecimal format.
 *
 * @param buffer The input buffer to be printed.
 *
 * @note This function prints the content of the input buffer in hexadecimal
 * format. If the buffer contains more than 5 bytes, it highlights the 6th and
 * 7th bytes by enclosing them in brackets.
 */
void printHexBuff(const std::vector<uint8_t>& buffer) {
  if (buffer.size() < 5) {
    // Buffer size is too small to process
    Serial.println("Buffer too small");
    return;
  }

  uint16_t data_size = buffer[3] << 8 | buffer[4];

  for (size_t i = 0; i < buffer.size(); ++i) {
    if (i == 5 && i + 1 < buffer.size()) {
      Serial.print("[");
      Serial.print(buffer[i] < 16 ? "0" : "");
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
      Serial.print(buffer[i + 1] < 16 ? "0" : "");
      Serial.print(buffer[i + 1], HEX);
      Serial.print("] ");
      ++i;  // Skip the next byte as it's already printed
    } else if (i > 7 && data_size > 0 && i + data_size <= buffer.size()) {
      Serial.print("[");
      size_t j = i;
      for (; j < i + data_size - 1; ++j) {
        Serial.print(buffer[j] < 16 ? "0" : "");
        Serial.print(buffer[j], HEX);
        Serial.print(" ");
      }
      Serial.print(buffer[j] < 16 ? "0" : "");
      Serial.print(buffer[j], HEX);
      Serial.print("] ");
      i += data_size - 1;  // Skip the printed bytes
      data_size = 0;
    } else {
      Serial.print(buffer[i] < 16 ? "0" : "");
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
  }
  Serial.println();
}

/**
 * @brief Calculate the expected frame length from the buffer.
 *
 * @param buffer The input buffer containing frame data.
 * @return The expected frame length.
 *
 * @note This function calculates the expected frame length based on the
 * buffer's content. It checks the buffer size to ensure it contains at least
 * the frame header, then calculates the total frame length by adding the data
 * length and checksum size to the frame header size.
 */
size_t SeeedmmWave::expectedFrameLength(const std::vector<uint8_t>& buffer) {
  if (buffer.size() < SIZE_FRAME_HEADER) {
    return SIZE_FRAME_HEADER;  // minimum frame header size
  }
  size_t len = (buffer[3] << 8) | buffer[4];
  return SIZE_FRAME_HEADER + len + SIZE_DATA_CKSUM;
}

/**
 * @brief Calculate the checksum for a byte array.
 *
 * This function calculates the checksum for the provided byte array using an
 * XOR-based checksum algorithm.
 *
 * @param data The byte array to calculate the checksum for.
 * @param len The length of the byte array.
 * @return The calculated checksum.
 */
uint8_t SeeedmmWave::calculateChecksum(const uint8_t* data, size_t len) {
  uint8_t checksum = 0;
  for (size_t i = 0; i < len; i++) {
    checksum ^= data[i];
  }
  checksum = ~checksum;
  return checksum;
}

/**
 * @brief Validate the checksum of a byte array.
 *
 * This function validates the checksum of the provided byte array by comparing
 * it to the expected checksum.
 *
 * @param data The byte array to validate.
 * @param len The length of the byte array.
 * @param expected_checksum The expected checksum.
 * @return True if the checksum is valid, false otherwise.
 */
bool SeeedmmWave::validateChecksum(const uint8_t* data, size_t len,
                                   uint8_t expected_checksum) {
  return calculateChecksum(data, len) == expected_checksum;
}

/**
 * @brief Convert a float value to a byte array.
 *
 * This function converts a float value to a byte array.
 *
 * @param value The float value to convert.
 * @param bytes The byte array to store the converted value.
 */
void SeeedmmWave::floatToBytes(float value, uint8_t* bytes) {
  union {
    float f;
    uint8_t bytes[sizeof(float)];
  } floatUnion;

  floatUnion.f = value;
#if SEEED_WAVE_IS_BIG_ENDIAN == 1
  for (size_t i = 0; i < sizeof(float); ++i) {
    bytes[i] = floatUnion.bytes[sizeof(float) - 1 - i];
  }
#else
  for (size_t i = 0; i < sizeof(float); ++i) {
    bytes[i] = floatUnion.bytes[i];
  }
#endif
}

/**
 * @brief Convert a 32-bit unsigned integer to a byte array.
 *
 * This function converts a 32-bit unsigned integer to a byte array.
 *
 * @param value The 32-bit unsigned integer to convert.
 * @param bytes The byte array to store the converted value.
 */
void SeeedmmWave::uint32ToBytes(uint32_t value, uint8_t* bytes) {
  uint8_t* p = reinterpret_cast<uint8_t*>(&value);
  for (size_t i = 0; i < sizeof(uint32_t); ++i) {
    bytes[i] = p[i];
  }
}

/**
 * @brief Extract a float value from a byte array.
 *
 * @param bytes The byte array containing the float value.
 * @return The extracted float value.
 */
float SeeedmmWave::extractFloat(const uint8_t* bytes) const {
  return *reinterpret_cast<const float*>(bytes);
}

/**
 * @brief Extract a 32-bit unsigned integer from a byte array.
 *
 * This function extracts a 32-bit unsigned integer from the provided byte
 * array.
 *
 * @param bytes The byte array containing the 32-bit unsigned integer.
 * @return The extracted 32-bit unsigned integer.
 */
uint32_t SeeedmmWave::extractU32(const uint8_t* bytes) const {
  return *reinterpret_cast<const uint32_t*>(bytes);
}

/**
 * @brief Initialize the SeeedmmWave object.
 *
 *
 * @param serial Pointer to the hardware serial object.
 * @param baud The baud rate for the serial communication.
 * @param wait_delay The delay time to wait for the sensor.
 * @param rst The reset pin number. If negative, no reset is performed.
 *
 * @note This function initializes the SeeedmmWave object by setting the serial
 * port, baud rate, wait delay, and optionally resetting the hardware.
 */
void SeeedmmWave::begin(HardwareSerial* serial, uint32_t baud,
                        uint32_t wait_delay, int rst) {
  this->_serial     = serial;
  this->_baud       = baud;
  this->_wait_delay = wait_delay;

  _serial->setRxBufferSize(1024 * 32);
  _serial->begin(_baud);
  _serial->setTimeout(1000);

  // _serial->setRxFIFOFull(20);
  if (rst >= 0) {
    pinMode(rst, OUTPUT);
    digitalWrite(rst, LOW);
    delay(50);
    digitalWrite(rst, HIGH);
    delay(500);
  }
}

/**
 * @brief Check the availability of data on the serial port.
 *
 * This function returns the number of bytes available for reading from the
 * serial port.
 *
 * @return The number of bytes available.
 */
int SeeedmmWave::available() {
  return _serial ? _serial->available() : 0;
}

/**
 * @brief Read data from the serial port.
 *
 * @param data The buffer to store the read data.
 * @param length The number of bytes to read.
 * @return The number of bytes actually read.
 */
int SeeedmmWave::read(char* data, int length) {
  return _serial ? _serial->readBytes(data, length) : 0;
}

int SeeedmmWave::read(void) {
  return _serial ? _serial->read() : 0;
}

size_t SeeedmmWave::write(const uint8_t* buffer, size_t size) {
  return _serial ? _serial->write(buffer, size) : 0;
}

size_t SeeedmmWave::write(const char* buffer, size_t size) {
  return _serial ? _serial->write(buffer, size) : 0;
}

/**
 * @brief Process a received frame of data.
 *
 * @attention This function processes a received frame of data, validating the
 * checksums and handling the frame based on its type.
 *
 * @param frame_bytes The byte array containing the frame data.
 * @param len The length of the frame data.
 * @param data_type The expected data type of the frame. Defaults to 0xFFFF.
 * @return True if the frame is successfully processed, false otherwise.
 */
bool SeeedmmWave::processFrame(const uint8_t* frame_bytes, size_t len,
                               uint16_t data_type) {
  if (len < SIZE_FRAME_HEADER)
    return false;  // Not enough data to process header

  uint16_t id        = (frame_bytes[1] << 8) | frame_bytes[2];
  uint16_t data_len  = (frame_bytes[3] << 8) | frame_bytes[4];
  uint16_t type      = (frame_bytes[5] << 8) | frame_bytes[6];
  uint8_t head_cksum = frame_bytes[7];
  uint8_t data_cksum = frame_bytes[SIZE_FRAME_HEADER + data_len];

  // Only proceed if the type matches or if data_type is set to the default,
  // indicating no specific type is required
  if (data_type != 0xFFFF && data_type != type)
    return false;
  // Checksum validation6
  if (!validateChecksum(frame_bytes, SIZE_FRAME_HEADER - SIZE_DATA_CKSUM,
                        head_cksum) ||
      !validateChecksum(&frame_bytes[SIZE_FRAME_HEADER], data_len,
                        data_cksum)) {
    return false;
  }

  return handleType(type, &frame_bytes[SIZE_FRAME_HEADER], data_len);
}

std::vector<uint8_t> SeeedmmWave::packetFrame(uint16_t type,
                                              const uint8_t* data, size_t len) {
  static uint16_t _id = 0x8000;
  std::vector<uint8_t>
      frame;  // SOF, ID, LEN, TYPE, HEAD_CKSUM, DATA, DATA_CKSUM

  frame.push_back(SOF_BYTE);     // Start of Frame
  frame.push_back(_id >> 8);     // ID high byte
  frame.push_back(_id & 0xFF);   // ID low byte
  frame.push_back(len >> 8);     // Length high byte
  frame.push_back(len & 0xFF);   // Length low byte
  frame.push_back(type >> 8);    // Type high byte
  frame.push_back(type & 0xFF);  // Type low byte

  uint8_t head_cksum = calculateChecksum(frame.data(), frame.size());
  frame.push_back(head_cksum);  // Header checksum

  if (data != nullptr) {
    frame.insert(frame.end(), data, data + len);  // Insert data

    uint8_t data_cksum = calculateChecksum(data, len);
    frame.push_back(data_cksum);  // Data checksum
  }

  _id++;
  return frame;
}

bool SeeedmmWave::sendFrame(const std::vector<uint8_t>& frame) {
#if _MMWAVE_DEBUG == 1
  Serial.print("Send<<<");
  printHexBuff(frame);
#endif

  size_t totalBytesSent = 0;
  size_t frameSize      = frame.size();
  const uint8_t* data   = frame.data();

  while (totalBytesSent < frameSize) {
    size_t bytesToSend = frameSize - totalBytesSent;
    size_t bytesSent   = _serial->write(data + totalBytesSent, bytesToSend);

    // Ensure all bytes are actually written
    if (bytesSent > 0) {
      totalBytesSent += bytesSent;
    } else {
      // Handle the case where no bytes are sent
      Serial.println("Error: No bytes sent, retrying...");
    }

    // Wait for the data to be sent
    _serial->flush();
  }

  return totalBytesSent == frameSize;
}

/**
 * @brief Send a frame of data.
 *
 * @attention This function constructs and sends a frame of data, including
 * the frame header, data, and checksums.
 *
 * @param type The type of the frame.
 * @param data The data to include in the frame. Defaults to nullptr.
 * @param len The length of the data. Defaults to 0.
 * @return True if the frame is sent successfully, false otherwise.
 */
bool SeeedmmWave::send(uint16_t type, const uint8_t* data, size_t data_len) {
  std::vector<uint8_t> frame = packetFrame(type, data, data_len);
  return sendFrame(frame);
}

void SeeedmmWave::fetch(uint32_t timeout) {
  static bool startFrame = false;
  static std::vector<uint8_t> frameBuffer;
  uint32_t expire_time = millis() + timeout;
  uint8_t frameDataSize;
  do {
    size_t c_available = _serial->available();
    while (c_available--) {
      uint8_t byte = _serial->read();
      if (startFrame)  // Frame processing
      {
        frameBuffer.push_back(byte);
        // Serial.print("Read byte: ");
        // Serial.println(byte, HEX);
        if (frameBuffer.size() >= SIZE_FRAME_HEADER)  // right package
        {
          frameDataSize = (frameBuffer[3] << 8 | frameBuffer[4]);
          if (frameDataSize > 30) {
            startFrame = false;
            // Serial.println("FrameDataSize too large, clearing buffer");
            continue;
          }
          if (frameBuffer.size() ==
              (SIZE_FRAME_HEADER + frameDataSize + SIZE_DATA_CKSUM)) {
            if (byteQueue.size() >= MMWaveMaxQueueSize) {
              byteQueue.pop();  // Discard the oldest frame
              // Serial.println("Queue full, discarding oldest frame");
            }
#if _MMWAVE_DEBUG == 1
            printHexBuff(frameBuffer);
#endif
            byteQueue.push(frameBuffer);  // Add the complete frame to the queue
            startFrame = false;
          }
        }
      } else {
        if (byte == SOF_BYTE) {  // Start of frame
          startFrame = true;
          frameBuffer.clear();
          frameBuffer.push_back(byte);  // insert 0x01
          // Serial.println("Start of Frame detected 0x01");
        }
      }
    }
  } while (millis() < expire_time);
}

bool SeeedmmWave::processQueuedFrames(uint16_t data_type, uint32_t timeout) {
  bool result = false;

  if (byteQueue.empty()) {
    return false;
  }

  uint32_t expire_time = millis() + timeout;
  do {
    std::vector<uint8_t> frame = byteQueue.front();
    byteQueue.pop();
#if _MMWAVE_DEBUG == 1
    printHexBuff(frame);  // Print received bytes
#endif
    if (!this->processFrame(frame.data(), frame.size(), data_type)) {
      continue;
    } else {
      result = true;
    }
  } while (!byteQueue.empty() && timeout);

  return result;
}

/**
 * @brief
 *
 * @param timeout better bigger than 10
 * @return true
 * @return false
 */
bool SeeedmmWave::update(uint32_t timeout) {
  this->fetch(timeout);
  return processQueuedFrames(0xFFFF, timeout);
}

bool SeeedmmWave::fetchType(uint16_t data_type, uint32_t timeout) {
  this->fetch(timeout);
  return processQueuedFrames(data_type);
}
