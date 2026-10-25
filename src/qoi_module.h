#include <godot_cpp/classes/image_format_loader_extension.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/file_access.hpp>

namespace godot {

class ImageQOI : public Image {
    GDCLASS(ImageQOI, Image)

private:
    Ref<ImageQOI> _ensure(int &r_channels);

protected:
    static void _bind_methods();

public:
    Error load_qoi_from_buffer(const PackedByteArray &p_array);
    Error save_qoi(const String &path);
    PackedByteArray save_qoi_to_buffer();
    static Ref<ImageQOI> from_image(const Ref<Image> &p_img);
};

class ImageFormatLoaderQOI : public ImageFormatLoaderExtension {
    GDCLASS(ImageFormatLoaderQOI, ImageFormatLoaderExtension)

protected:
    static void _bind_methods() {}

public:
    virtual PackedStringArray _get_recognized_extensions() const override;
    virtual Error _load_image(Ref<Image> p_image, Ref<FileAccess> p_file, BitField<ImageFormatLoader::LoaderFlags> _p_flags, float _p_scale) const;
};

} // namespace godot