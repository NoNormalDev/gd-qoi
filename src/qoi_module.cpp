#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "qoi_module.h"

#define QOI_MALLOC(p_size) memalloc(p_size)
#define QOI_FREE(p_ptr) memfree(p_ptr)
#define QOI_IMPLEMENTATION
#define QOI_NO_STDIO
#include "qoi.h"

namespace godot {

PackedStringArray ImageFormatLoaderQOI::_get_recognized_extensions() const {
    PackedStringArray arr;
    arr.push_back("qoi");
    return arr;
}

Error ImageFormatLoaderQOI::_load_image(Ref<Image> p_image, Ref<FileAccess> p_file, BitField<ImageFormatLoader::LoaderFlags> _p_flags, float _p_scale) const {
    ERR_FAIL_NULL_V(p_file, ERR_BUG);
    ERR_FAIL_NULL_V(p_image, ERR_BUG);

    PackedByteArray file_data = p_file->get_buffer(p_file->get_length());
    if (file_data.size() == 0 || p_file->get_error() != OK) {
        return ERR_FILE_CANT_READ;
    }

    qoi_desc desc;
    void *decoded = qoi_decode(file_data.ptr(), file_data.size(), &desc, 0);
    ERR_FAIL_NULL_V(decoded, ERR_FILE_CORRUPT);

    Image::Format format;
    if (desc.channels == 3) {
        format = Image::FORMAT_RGB8;
    } else if (desc.channels == 4) {
        format = Image::FORMAT_RGBA8;
    } else {
        QOI_FREE(decoded);
        return ERR_FILE_CORRUPT;
    }

    int64_t data_size = (int64_t)desc.width * (int64_t)desc.height * (int64_t)desc.channels;
    PackedByteArray pixel_data;
    pixel_data.resize(data_size);
    memcpy(pixel_data.ptrw(), decoded, pixel_data.size());
    QOI_FREE(decoded);

    p_image->set_data(
        desc.width, desc.height,
        false, format,
        pixel_data
    );

    return OK;
}

void ImageQOI::_bind_methods() {
    ClassDB::bind_method(D_METHOD("save_qoi", "path"), &ImageQOI::save_qoi);
    ClassDB::bind_method(D_METHOD("save_qoi_to_buffer"), &ImageQOI::save_qoi_to_buffer);
    ClassDB::bind_method(D_METHOD("load_qoi_from_buffer", "buffer"), &ImageQOI::load_qoi_from_buffer);
    ClassDB::bind_static_method("ImageQOI", D_METHOD("from_image", "img"), &ImageQOI::from_image);
}

Ref<ImageQOI> ImageQOI::from_image(const Ref<Image> &p_img) {
    ERR_FAIL_COND_V(p_img.is_null(), Ref<ImageQOI>());
    Ref<ImageQOI> result = memnew(ImageQOI);
    result->set_data(
        p_img->get_width(),
        p_img->get_height(),
        p_img->has_mipmaps(),
        p_img->get_format(),
        p_img->get_data()
    );
    return result;
}

Error ImageQOI::load_qoi_from_buffer(const PackedByteArray &p_array) {
    ERR_FAIL_COND_V(p_array.size() == 0, ERR_INVALID_PARAMETER);

    qoi_desc desc;
    void *decoded = qoi_decode(p_array.ptr(), p_array.size(), &desc, 0);
    ERR_FAIL_NULL_V(decoded, ERR_INVALID_DATA);

    Image::Format format;
    if (desc.channels == 3) {
        format = Image::FORMAT_RGB8;
    } else if (desc.channels == 4) {
        format = Image::FORMAT_RGBA8;
    } else {
        QOI_FREE(decoded);
        return ERR_INVALID_DATA;
    }

    int64_t data_size = (int64_t)desc.width * (int64_t)desc.height * (int64_t)desc.channels;
    PackedByteArray pixel_data;
    pixel_data.resize(data_size);
    memcpy(pixel_data.ptrw(), decoded, pixel_data.size());
    QOI_FREE(decoded);

    set_data(
        desc.width, desc.height,
        false, format,
        pixel_data
    );

    return OK;
}

Ref<ImageQOI> ImageQOI::_ensure(int &r_channels) {
    Image::Format format = get_format();
    switch (format) {
        case Image::FORMAT_RGB8:
            r_channels = 3;
            return this;
        case Image::FORMAT_RGBA8:
            r_channels = 4;
            return this;
        case Image::FORMAT_L8:
        case Image::FORMAT_R8:
        case Image::FORMAT_RG8:
        case Image::FORMAT_RGB565:
        case Image::FORMAT_RF:
        case Image::FORMAT_RGF:
        case Image::FORMAT_RGBF:
        case Image::FORMAT_RH:
        case Image::FORMAT_RGH:
        case Image::FORMAT_RGBH:
        case Image::FORMAT_RGBE9995:
        case Image::FORMAT_RGTC_R:
        case Image::FORMAT_RGTC_RG:
        case Image::FORMAT_BPTC_RGBF:
        case Image::FORMAT_BPTC_RGBFU:
        case Image::FORMAT_ETC:
        case Image::FORMAT_ETC2_R11:
        case Image::FORMAT_ETC2_R11S:
        case Image::FORMAT_ETC2_RG11:
        case Image::FORMAT_ETC2_RG11S:
        case Image::FORMAT_ETC2_RGB8:
        case Image::FORMAT_DXT5_RA_AS_RG:
            r_channels = 3;
            break;
        default:
            r_channels = 4;
            break;
    }
    Ref<ImageQOI> newimg = memnew(ImageQOI);
    newimg->set_data(get_width(), get_height(), has_mipmaps(), get_format(), get_data().duplicate());
    newimg->convert(r_channels == 4 ? Image::FORMAT_RGBA8 : Image::FORMAT_RGB8);
    return newimg;
}

Error ImageQOI::save_qoi(const String &path) {
    ERR_FAIL_COND_V(is_empty(), ERR_UNCONFIGURED);
    ERR_FAIL_COND_V(path.is_empty(), ERR_INVALID_PARAMETER);

    int channels = 0;
    Ref<ImageQOI> img = _ensure(channels);
    ERR_FAIL_COND_V(channels != 3 && channels != 4, ERR_BUG);

    Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
    ERR_FAIL_COND_V(file.is_null(), ERR_FILE_CANT_OPEN);

    int size = -1;
    qoi_desc desc;
    desc.width = img->get_width();
    desc.height = img->get_height();
    desc.channels = channels;
    desc.colorspace = QOI_SRGB;

    void *encoded = qoi_encode(img->get_data().ptr(), &desc, &size);
    ERR_FAIL_NULL_V(encoded, FAILED);

    if (size <= 0) {
        QOI_FREE(encoded);
        ERR_FAIL_COND_V(size <= 0, FAILED);
    }

    file->store_buffer((const uint8_t *)encoded, size);
    QOI_FREE(encoded);

    file->close();

    return OK;
}

PackedByteArray ImageQOI::save_qoi_to_buffer() {
    ERR_FAIL_COND_V(is_empty(), PackedByteArray());

    int channels = 0;
    Ref<ImageQOI> img = _ensure(channels);
    ERR_FAIL_COND_V(channels != 3 && channels != 4, PackedByteArray());

    int size = -1;
    qoi_desc desc;
    desc.width = img->get_width();
    desc.height = img->get_height();
    desc.channels = channels;
    desc.colorspace = QOI_SRGB;

    void *encoded = qoi_encode(img->get_data().ptr(), &desc, &size);
    ERR_FAIL_NULL_V(encoded, PackedByteArray());

    if (size <= 0) {
        QOI_FREE(encoded);
        ERR_FAIL_COND_V(size <= 0, PackedByteArray());
    }

    PackedByteArray buffer;
    buffer.resize(size);
    memcpy(buffer.ptrw(), encoded, size);
    QOI_FREE(encoded);

    return buffer;
}

} // namespace godot