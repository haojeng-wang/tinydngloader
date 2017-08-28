//
// TinyDNGWriter, single header only DNG writer.
//

/*
The MIT License (MIT)

Copyright (c) 2016 - 2017 Syoyo Fujita.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef TINY_DNG_WRITER_H_
#define TINY_DNG_WRITER_H_

#include <sstream>
#include <vector>

namespace tinydngwriter {

typedef enum {
  TIFFTAG_SUB_FILETYPE = 254,
  TIFFTAG_IMAGE_WIDTH = 256,
  TIFFTAG_IMAGE_LENGTH = 257,
  TIFFTAG_BITS_PER_SAMPLE = 258,
  TIFFTAG_COMPRESSION = 259,
  TIFFTAG_PHOTOMETRIC = 262,
  TIFFTAG_STRIP_OFFSET = 273,
  TIFFTAG_SAMPLES_PER_PIXEL = 277,
  TIFFTAG_ROWS_PER_STRIP = 278,
  TIFFTAG_STRIP_BYTE_COUNTS = 279,
  TIFFTAG_PLANAR_CONFIG = 284,
  TIFFTAG_ORIENTATION = 274,

  // DNG extension
  TIFFTAG_CFA_REPEAT_PATTERN_DIM = 33421,
  TIFFTAG_CFA_PATTERN = 33422,

  TIFFTAG_DNG_VERSION = 50706,
  TIFFTAG_DNG_BACKWARD_VERSION = 50707,
  TIFFTAG_CHRROMA_BLUR_RADIUS = 50703,
  TIFFTAG_BLACK_LEVEL = 50714,
  TIFFTAG_WHITE_LEVEL = 50717,
  TIFFTAG_COLOR_MATRIX1 = 50721,
  TIFFTAG_COLOR_MATRIX2 = 50722,
  TIFFTAG_EXTRA_CAMERA_PROFILES = 50933,
  TIFFTAG_PROFILE_NAME = 50936,
  TIFFTAG_AS_SHOT_PROFILE_NAME = 50934,
  TIFFTAG_DEFAULT_BLACK_RENDER = 51110,
  TIFFTAG_ACTIVE_AREA = 50829,
  TIFFTAG_FORWARD_MATRIX1 = 50964,
  TIFFTAG_FORWARD_MATRIX2 = 50965
} Tag;

// SUBFILETYPE
static const int FILETYPE_REDUCEDIMAGE = 1;
static const int FILETYPE_PAGE = 2;
static const int FILETYPE_MASK = 4;

// PLANARCONFIG
static const int PLANARCONFIG_CONTIG = 1;
static const int PLANARCONFIG_SEPARATE = 2;

// COMPRESSION
// TODO(syoyo) more compressin types.
static const int COMPRESSION_NONE = 1;

// ORIENTATION
static const int ORIENTATION_TOPLEFT = 1;
static const int ORIENTATION_TOPRIGHT = 2;
static const int ORIENTATION_BOTRIGHT = 3;
static const int ORIENTATION_BOTLEFT = 4;
static const int ORIENTATION_LEFTTOP = 5;
static const int ORIENTATION_RIGHTTOP = 6;
static const int ORIENTATION_RIGHTBOT = 7;
static const int ORIENTATION_LEFTBOT = 8;

// PHOTOMETRIC
// TODO(syoyo): more photometric types.
static const int PHOTOMETRIC_RGB = 2;            // Default
static const int PHOTOMETRIC_CFA = 32893;        // DNG ext
static const int PHOTOMETRIC_LINEARRAW = 34892;  // DNG ext

struct IFDTag {
  unsigned short tag;
  unsigned short type;
  unsigned int count;
  unsigned int offset_or_value;
};
// 12 bytes.

class DNGImage {
 public:
  DNGImage();
  ~DNGImage() {}

  /// Optional: Explicitly specify endian swapness.
  bool SwapEndian(bool swap_endian);

  bool SetSubfileType(unsigned int value);
  bool SetImageWidth(unsigned int value);
  bool SetImageLength(unsigned int value);
  bool SetRowsPerStrip(unsigned int value);
  bool SetSamplesPerPixel(unsigned short value);
  bool SetBitsPerSample(unsigned short value);
  bool SetPhotometric(unsigned short value);
  bool SetPlanarConfig(unsigned short value);
  bool SetOrientation(unsigned short value);
  bool SetCompression(unsigned short value);

  bool SetActiveArea(const unsigned int values[4]);

  bool SetChromaBlurRadius(double value);

  /// Specify black level per sample.
  bool SetBlackLevelRational(unsigned int num_samples, const double *values);

  /// Specify white level per sample.
  bool SetWhiteLevelRational(unsigned int num_samples, const double *values);

  /// Set image data
  bool SetImageData(const unsigned char *data, const size_t data_len);

  /// Set custom field
  bool SetCustomFieldLong(const unsigned short tag, const int value);
  bool SetCustomFieldULong(const unsigned short tag, const unsigned int value);

  size_t GetDataSize() const { return data_os_.str().length(); }

  bool WriteDataToStream(std::ostream *ofs, std::string *err) const;
  bool WriteIFDToStream(const unsigned int ifd_base_offset, std::ostream *ofs,
                        std::string *err) const;

 private:
  std::ostringstream data_os_;
  bool is_host_big_endian_;
  bool swap_endian_;
  unsigned short num_fields_;
  unsigned int samples_per_pixels_;

  std::vector<IFDTag> ifd_tags_;
};

class DNGWriter {
 public:
  DNGWriter();
  ~DNGWriter() {}

  /// Optional: Explicitly specify endian swapness.
  bool SwapEndian(bool swap_endian);

  ///
  /// Add DNGImage.
  /// It just retains the pointer of the image, thus
  /// application must not free resources until `WriteToFile` has been called.
  ///
  bool AddImage(const DNGImage *image) {
    images_.push_back(image);

    return true;
  }

  /// Write DNG to a file.
  /// Return error string to `err` when Write() returns false.
  /// Returns true upon success.
  bool WriteToFile(const char *filename, std::string *err) const;

 private:
  bool is_host_big_endian_;
  bool swap_endian_;
  char pad[6];

  std::vector<const DNGImage *> images_;
};

}  // namespace tinydng

#endif  // TINY_DNG_WRITER_H_

#ifdef TINY_DNG_WRITER_IMPLEMENTATION

//
// TIFF format resources.
//
// http://c0de517e.blogspot.jp/2013/07/tiny-hdr-writer.html
// http://paulbourke.net/dataformats/tiff/ and
// http://partners.adobe.com/public/developer/en/tiff/TIFF6.pdf
//

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

namespace tinydngwriter {

//
// TinyDNGWriter stores IFD table in the end of file so that offset to
// image data can be easily computed.
//
// +----------------------+
// |    header            |
// +----------------------+
// |                      |
// |  image & meta 0      |
// |                      |
// +----------------------+
// |                      |
// |  image & meta 1      |
// |                      |
// +----------------------+
//    ...
// +----------------------+
// |                      |
// |  image & meta N      |
// |                      |
// +----------------------+
// |                      |
// |  IFD 0               |
// |                      |
// +----------------------+
// |                      |
// |  IFD 1               |
// |                      |
// +----------------------+
//    ...
// +----------------------+
// |                      |
// |  IFD 2               |
// |                      |
// +----------------------+
//

// From tiff.h
typedef enum {
  TIFF_NOTYPE = 0,     /* placeholder */
  TIFF_BYTE = 1,       /* 8-bit unsigned integer */
  TIFF_ASCII = 2,      /* 8-bit bytes w/ last byte null */
  TIFF_SHORT = 3,      /* 16-bit unsigned integer */
  TIFF_LONG = 4,       /* 32-bit unsigned integer */
  TIFF_RATIONAL = 5,   /* 64-bit unsigned fraction */
  TIFF_SBYTE = 6,      /* !8-bit signed integer */
  TIFF_UNDEFINED = 7,  /* !8-bit untyped data */
  TIFF_SSHORT = 8,     /* !16-bit signed integer */
  TIFF_SLONG = 9,      /* !32-bit signed integer */
  TIFF_SRATIONAL = 10, /* !64-bit signed fraction */
  TIFF_FLOAT = 11,     /* !32-bit IEEE floating point */
  TIFF_DOUBLE = 12,    /* !64-bit IEEE floating point */
  TIFF_IFD = 13,       /* %32-bit unsigned integer (offset) */
  TIFF_LONG8 = 16,     /* BigTIFF 64-bit unsigned integer */
  TIFF_SLONG8 = 17,    /* BigTIFF 64-bit signed integer */
  TIFF_IFD8 = 18       /* BigTIFF 64-bit unsigned integer (offset) */
} DataType;

const static int kHeaderSize = 8;  // TIFF header size.

static inline bool IsBigEndian() {
  unsigned int i = 0x01020304;
  char c[4];
  memcpy(c, &i, 4);
  return (c[0] == 1);
}

static void swap2(unsigned short *val) {
  unsigned short tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[1];
  dst[1] = src[0];
}

static void swap4(unsigned int *val) {
  unsigned int tmp = *val;
  unsigned char *dst = reinterpret_cast<unsigned char *>(val);
  unsigned char *src = reinterpret_cast<unsigned char *>(&tmp);

  dst[0] = src[3];
  dst[1] = src[2];
  dst[2] = src[1];
  dst[3] = src[0];
}

static void Write1(const unsigned char c, std::ostringstream *out) {
  unsigned char value = c;
  out->write(reinterpret_cast<const char *>(&value), 1);
}

static void Write2(const unsigned short c, std::ostringstream *out,
                   const bool swap_endian) {
  unsigned short value = c;
  if (swap_endian) {
    swap2(&value);
  }

  out->write(reinterpret_cast<const char *>(&value), 2);
}

static void Write4(const unsigned int c, std::ostringstream *out,
                   const bool swap_endian) {
  unsigned int value = c;
  if (swap_endian) {
    swap4(&value);
  }

  out->write(reinterpret_cast<const char *>(&value), 4);
}

static bool WriteTIFFTag(const unsigned short tag, const unsigned short type,
                         const unsigned int count, const unsigned char *data,
                         std::vector<IFDTag> *tags_out,
                         std::ostringstream *data_out) {
  assert(sizeof(IFDTag) ==
         12);  // FIXME(syoyo): Use static_assert for C++11 compiler

  IFDTag ifd;
  ifd.tag = tag;
  ifd.type = type;
  ifd.count = count;

  size_t typesize_table[] = {1, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8, 4};

  size_t len = count * (typesize_table[(type) < 14 ? (type) : 0]);
  if (len > 4) {
    // Store offset value.

    unsigned int offset =
        static_cast<unsigned int>(data_out->tellp()) + kHeaderSize;
    ifd.offset_or_value = offset;

    data_out->write(reinterpret_cast<const char *>(data),
                    static_cast<ssize_t>(len));

  } else {
    ifd.offset_or_value = 0;

    // less than 4 bytes = store data itself.
    if (len == 1) {
      unsigned char value = *(data);
      memcpy(&(ifd.offset_or_value), &value, sizeof(unsigned char));
    } else if (len == 2) {
      unsigned short value = *(reinterpret_cast<const unsigned short *>(data));
      memcpy(&(ifd.offset_or_value), &value, sizeof(unsigned short));
    } else if (len == 4) {
      unsigned int value = *(reinterpret_cast<const unsigned int *>(data));
      ifd.offset_or_value = value;
    } else {
      assert(0);
    }
  }

  tags_out->push_back(ifd);

  return true;
}

static bool WriteTIFFVersionHeader(std::ostringstream *out) {
  // TODO(syoyo): Support BigTIFF?

  // 4d 4d = Big endian. 49 49 = Little endian.
  Write1(0x4d, out);
  Write1(0x4d, out);
  Write1(0x0, out);
  Write1(0x2a, out);  // Tiff version ID

  return true;
}

DNGImage::DNGImage()
    : is_host_big_endian_(false),
      swap_endian_(true),
      num_fields_(0),
      samples_per_pixels_(0) {
  // Data is stored in big endian, thus no byteswapping required for big endian
  // machine.
  if (IsBigEndian()) {
    swap_endian_ = false;
  }
}

bool DNGImage::SetSubfileType(const unsigned int value) {
  unsigned int count = 1;

  unsigned int data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_SUB_FILETYPE), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageWidth(const unsigned int width) {
  unsigned int count = 1;

  unsigned int data = width;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_IMAGE_WIDTH), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageLength(const unsigned int length) {
  unsigned int count = 1;

  const unsigned int data = length;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_IMAGE_LENGTH), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetRowsPerStrip(const unsigned int rows) {
  if (rows == 0) {
    return false;
  }

  unsigned int count = 1;

  const unsigned int data = rows;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_ROWS_PER_STRIP), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetSamplesPerPixel(const unsigned short value) {
  if (value > 4) {
    return false;
  }

  unsigned int count = 1;

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_SAMPLES_PER_PIXEL), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  samples_per_pixels_ = value;  // Store SPP for later use.

  num_fields_++;
  return true;
}

bool DNGImage::SetBitsPerSample(const unsigned short value) {
  if (value > 32) {
    return false;
  }

  unsigned int count = 1;

  const unsigned short data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_BITS_PER_SAMPLE), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetPhotometric(const unsigned short value) {
  if ((value == PHOTOMETRIC_LINEARRAW) || (value == PHOTOMETRIC_RGB)) {
    // OK
  } else {
    return false;
  }

  unsigned int count = 1;

  const unsigned int data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_PHOTOMETRIC), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetPlanarConfig(const unsigned short value) {
  unsigned int count = 1;

  if ((value == PLANARCONFIG_CONTIG) || (value == PLANARCONFIG_SEPARATE)) {
    // OK
  } else {
    return false;
  }

  const unsigned int data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_PLANAR_CONFIG), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCompression(const unsigned short value) {
  unsigned int count = 1;

  if ((value == COMPRESSION_NONE)) {
    // OK
  } else {
    return false;
  }

  const unsigned int data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_COMPRESSION), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetOrientation(const unsigned short value) {
  unsigned int count = 1;

  if ((value == ORIENTATION_TOPLEFT) || (value == ORIENTATION_TOPRIGHT) ||
      (value == ORIENTATION_BOTRIGHT) || (value == ORIENTATION_BOTLEFT) ||
      (value == ORIENTATION_LEFTTOP) || (value == ORIENTATION_RIGHTTOP) ||
      (value == ORIENTATION_RIGHTBOT) || (value == ORIENTATION_LEFTBOT)) {
    // OK
  } else {
    return false;
  }

  const unsigned int data = value;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_ORIENTATION), TIFF_SHORT, count,
      reinterpret_cast<const unsigned char *>(&data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetBlackLevelRational(unsigned int num_samples,
                                     const double *values) {
  // `SetSamplesPerPixel()` must be called in advance and SPP shoud be equal to
  // `num_samples`.
  if ((num_samples > 0) && (num_samples == samples_per_pixels_)) {
    // OK
  } else {
    return false;
  }

  unsigned int count = num_samples;

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_BLACK_LEVEL), TIFF_RATIONAL, count,
      reinterpret_cast<const unsigned char *>(values), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetWhiteLevelRational(unsigned int num_samples,
                                     const double *values) {
  // `SetSamplesPerPixel()` must be called in advance and SPP shoud be equal to
  // `num_samples`.
  if ((num_samples > 0) && (num_samples == samples_per_pixels_)) {
    // OK
  } else {
    return false;
  }

  unsigned int count = num_samples;

  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_WHITE_LEVEL), TIFF_RATIONAL, count,
      reinterpret_cast<const unsigned char *>(values), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetActiveArea(const unsigned int values[4]) {
  unsigned int count = 4;

  const unsigned int *data = values;
  bool ret = WriteTIFFTag(
      static_cast<unsigned short>(TIFFTAG_ACTIVE_AREA), TIFF_LONG, count,
      reinterpret_cast<const unsigned char *>(data), &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetImageData(const unsigned char *data, const size_t data_len) {
  if ((data == NULL) || (data_len < 1)) {
    return false;
  }

  const unsigned int offset =
      static_cast<unsigned int>(data_os_.str().length()) + kHeaderSize;

  data_os_.write(reinterpret_cast<const char *>(data),
                 static_cast<ssize_t>(data_len));

  {
    unsigned int count = 1;

    bool ret = WriteTIFFTag(static_cast<unsigned short>(TIFFTAG_STRIP_OFFSET),
                            TIFF_LONG, count,
                            reinterpret_cast<const unsigned char *>(&offset),
                            &ifd_tags_, &data_os_);

    if (!ret) {
      return false;
    }

    num_fields_++;
  }

  {
    unsigned int count = 1;
    unsigned int bytes = static_cast<unsigned int>(data_len);

    bool ret = WriteTIFFTag(
        static_cast<unsigned short>(TIFFTAG_STRIP_BYTE_COUNTS), TIFF_LONG,
        count, reinterpret_cast<const unsigned char *>(&bytes), &ifd_tags_,
        &data_os_);

    if (!ret) {
      return false;
    }

    num_fields_++;
  }

  return true;
}

bool DNGImage::SetCustomFieldLong(const unsigned short tag, const int value) {
  unsigned int count = 1;

  // TODO(syoyo): Check if `tag` value does not conflict with existing TIFF tag
  // value.

  bool ret = WriteTIFFTag(tag, TIFF_SLONG, count,
                          reinterpret_cast<const unsigned char *>(&value),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

bool DNGImage::SetCustomFieldULong(const unsigned short tag,
                                   const unsigned int value) {
  unsigned int count = 1;

  // TODO(syoyo): Check if `tag` value does not conflict with existing TIFF tag
  // value.

  bool ret = WriteTIFFTag(tag, TIFF_LONG, count,
                          reinterpret_cast<const unsigned char *>(&value),
                          &ifd_tags_, &data_os_);

  if (!ret) {
    return false;
  }

  num_fields_++;
  return true;
}

static bool IFDComparator(const IFDTag &a, const IFDTag &b) {
  return (a.tag < b.tag);
}

bool DNGImage::WriteDataToStream(std::ostream *ofs, std::string *err) const {
  if ((data_os_ == 0)) {
    if (err) {
      (*err) += "Empty image data.\n";
    }
    return false;
  }

  ofs->write(data_os_.str().c_str(),
             static_cast<ssize_t>(data_os_.str().length()));

  return true;
}

bool DNGImage::WriteIFDToStream(const unsigned int ifd_base_offset,
                                std::ostream *ofs, std::string *err) const {
  if ((num_fields_ == 0) || (ifd_tags_.size() < 1)) {
    if (err) {
      (*err) += "No TIFF Tags.\n";
    }
    return false;
  }

  std::ostringstream ifd_os;

  Write2(num_fields_, &ifd_os, swap_endian_);

  // Sort and write IFD tags.
  // TIFF expects IFD tags are sorted.
  {
    size_t typesize_table[] = {1, 1, 1, 2, 4, 8, 1, 1, 2, 4, 8, 4, 8, 4};

    std::vector<IFDTag> tags = ifd_tags_;

    std::sort(tags.begin(), tags.end(), IFDComparator);
    for (size_t i = 0; i < tags.size(); i++) {
      const IFDTag &ifd = tags[i];
      Write2(ifd.tag, &ifd_os, swap_endian_);
      Write2(ifd.type, &ifd_os, swap_endian_);
      Write4(ifd.count, &ifd_os, swap_endian_);

      size_t len =
          ifd.count * (typesize_table[(ifd.type) < 14 ? (ifd.type) : 0]);
      if (len > 4) {
        // Store offset value.
        unsigned int ifd_offt = ifd.offset_or_value + ifd_base_offset;
        Write4(ifd_offt, &ifd_os, swap_endian_);
      } else {
        // less than 4 bytes = store data itself.
        if (len == 1) {
          const unsigned char value =
              *(reinterpret_cast<const unsigned char *>(&ifd.offset_or_value));
          Write1(value, &ifd_os);
          unsigned char pad = 0;
          Write1(pad, &ifd_os);
          Write1(pad, &ifd_os);
          Write1(pad, &ifd_os);
        } else if (len == 2) {
          const unsigned short value =
              *(reinterpret_cast<const unsigned short *>(&ifd.offset_or_value));
          Write2(value, &ifd_os, swap_endian_);
          const unsigned short pad = 0;
          Write2(pad, &ifd_os, swap_endian_);
        } else if (len == 4) {
          const unsigned int value =
              *(reinterpret_cast<const unsigned int *>(&ifd.offset_or_value));
          Write4(value, &ifd_os, swap_endian_);
        } else {
          assert(0);
        }
      }
    }

    ofs->write(ifd_os.str().c_str(),
               static_cast<ssize_t>(ifd_os.str().length()));
  }

  return true;
}

// -------------------------------------------

DNGWriter::DNGWriter() : is_host_big_endian_(false), swap_endian_(true) {
  // Data is stored in big endian, thus no byteswapping required for big endian
  // machine.
  if (IsBigEndian()) {
    swap_endian_ = false;
  }
}

bool DNGWriter::WriteToFile(const char *filename, std::string *err) const {
  std::ofstream ofs(filename, std::ostream::binary);

  if (!ofs) {
    if (err) {
      (*err) = "Failed to open file.\n";
    }

    return false;
  }

  std::ostringstream header;
  bool ret = WriteTIFFVersionHeader(&header);
  if (!ret) {
    return false;
  }

  // 1. Compute offset and data size
  size_t data_len = 0;
  std::vector<size_t> offset_table;
  for (size_t i = 0; i < images_.size(); i++) {
    offset_table.push_back(data_len);
    data_len += images_[i]->GetDataSize();
  }

  // 2. Write offset to ifd table.
  const unsigned int ifd_offset =
      kHeaderSize + static_cast<unsigned int>(data_len);
  Write4(ifd_offset, &header, swap_endian_);

  assert(header.str().length() == 8);

  // std::cout << "ifd_offset " << ifd_offset << std::endl;
  // std::cout << "data_len " << data_os_.str().length() << std::endl;
  // std::cout << "ifd_len " << ifd_os_.str().length() << std::endl;
  // std::cout << "swap endian " << swap_endian_ << std::endl;

  // 3. Write header
  ofs.write(header.str().c_str(), static_cast<ssize_t>(header.str().length()));

  // 4. Write image and meta data
  for (size_t i = 0; i < images_.size(); i++) {
    bool ok = images_[i]->WriteDataToStream(&ofs, err);
    if (!ok) {
      return false;
    }
  }

  // 5. Write IFD entries;
  for (size_t i = 0; i < images_.size(); i++) {
    bool ok = images_[i]->WriteIFDToStream(
        static_cast<unsigned int>(offset_table[i]), &ofs, err);
    if (!ok) {
      return false;
    }

    unsigned int next_ifd_offset =
        static_cast<unsigned int>(ofs.tellp()) + sizeof(unsigned int);

    if (i == (images_.size() - 1)) {
      // Write zero as IFD offset(= end of data)
      next_ifd_offset = 0;
    }

    if (swap_endian_) {
      swap4(&next_ifd_offset);
    }

    ofs.write(reinterpret_cast<const char *>(&next_ifd_offset), 4);
  }

  return true;
}

}  // namespace tinydng

#endif  // TINY_DNG_WRITER_IMPLEMENTATION
