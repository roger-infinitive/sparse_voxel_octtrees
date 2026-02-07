#ifndef _INF_IMAGE_H_
#define _INF_IMAGE_H_

// TODO(roger): BC7 works on modern GPUs. If it doesn't work, then we need to fallback
// and decompress to RGBA8 when loading textures. Add a SupportsTextureFormat function to renderer.

#include "utility.h"
#include "bc7enc.c"
#include "bc7decomp.cpp"

struct RGBA8_Texture {
    u8* data;
    int width, height;
};

enum DDSFormat {
    DDSFormat_BC7_UNORM = 98,
};

#pragma pack(push, 1) //NOTE(roger): Packed to ensure there's no added padding between members.
struct DDSHeader {
    char filecode[4];    // "DDS "
    u32 size;            // Size of the header
    u32 flags;
    u32 height;
    u32 width;
    u32 pitchOrLinearSize;
    u32 depth;
    u32 mipMapCount;
    u32 reserved1[11];
    struct {
        u32 size;
        u32 flags;
        char fourCC[4];
        u32 RGBBitCount; // Number of bits in a pixel
        u32 RBitMask;    // Red bit mask
        u32 GBitMask;    // Green bit mask
        u32 BBitMask;    // Blue bit mask
        u32 ABitMask;    // Alpha bit mask
    } ddspf;
    u32 caps;
    u32 caps2;
    u32 caps3;
    u32 caps4;
    u32 reserved2;
};
struct DDSHeaderDXT10 {
    DDSFormat format;
    int resourceDimension;
    u32 miscFlag;
    u32 arraySize;
    u32 miscFlags2;
};
#pragma pack(pop)

void ApplyPremultiplyAlphaToDDS(const char* filePath) {
    TempArenaMemory tempMemory = TempArenaMemoryBegin(&tempAllocator);

    MemoryBuffer bc7Data = {};
    ReadEntireFileAndNullTerminate(filePath, &bc7Data, TempAllocator);

    DDSHeader* header = (DDSHeader*)bc7Data.buffer;
    u8* imageData = 0;

    const int PIXELS_PER_BLOCK = 16;
    bc7decomp::color_rgba unpackedPixels[PIXELS_PER_BLOCK];
    memset(unpackedPixels, 0, sizeof(unpackedPixels));

    if (strcmp(header->ddspf.fourCC, "DX10") == 0) {
        DDSHeaderDXT10* dx10Header = (DDSHeaderDXT10*)(bc7Data.buffer + sizeof(DDSHeader));

        if (dx10Header->format == DDSFormat_BC7_UNORM) {

            bc7enc_compress_block_init();
            bc7enc_compress_block_params params;
            bc7enc_compress_block_params_init(&params);

            imageData = (u8*)bc7Data.buffer + sizeof(DDSHeader) + sizeof(DDSHeaderDXT10);

            int blocksX = (header->width  + 3) / 4;
            int blocksY = (header->height + 3) / 4;

            for (int by = 0; by < blocksY; by++) {
                for (int bx = 0; bx < blocksX; bx++) {
                    int blockIndex = bx + by * blocksX;
                    u8* pBlock = &imageData[blockIndex * PIXELS_PER_BLOCK];
                    bc7decomp::unpack_bc7(pBlock, unpackedPixels);

                    for (int p = 0; p < PIXELS_PER_BLOCK; p++) {
                        bc7decomp::color_rgba* rgba = &unpackedPixels[p];

                        if (rgba->a == 0) {
                            memset(rgba, 0, sizeof(bc7decomp::color_rgba));
                        } else {
                            float alpha = rgba->a / 255.0f;
                            rgba->r = (u8)(rgba->r * alpha);
                            rgba->g = (u8)(rgba->g * alpha);
                            rgba->b = (u8)(rgba->b * alpha);
                        }
                    }

                    memset(pBlock, 0, 16);
                    bc7enc_compress_block(pBlock, (u8*)&unpackedPixels[0].r, &params);
                }
            }

            File outputFile = FileOpen(filePath, FileMode_Write);
            FileWrite(outputFile, bc7Data.buffer, bc7Data.size);
            FileClose(outputFile);

        } else {
            ASSERT_DEBUG(false, "UnpackRGBAFromDDS does not support format: %d", (int)dx10Header->format);
            goto unpack_exit;
        }
    } else {
        NOT_IMPLEMENTED();
        goto unpack_exit;
    }

unpack_exit:
    TempArenaMemoryEnd(tempMemory);
}

// loads main texture (ignores mips) and converts into an RGBA8 format.
void load_rgba8_from_dds_bc7(MemoryBuffer* dds, RGBA8_Texture* output, AllocFunc alloc) {
    DDSHeader* header = (DDSHeader*)dds->buffer;
    output->width = header->width;
    output->height = header->height;

    const int PIXELS_PER_BLOCK = 16;
    bc7decomp::color_rgba unpackedPixels[PIXELS_PER_BLOCK];

    bc7enc_compress_block_init();
    bc7enc_compress_block_params params;
    bc7enc_compress_block_params_init(&params);

    u8* imageData = (u8*)dds->buffer + sizeof(DDSHeader) + sizeof(DDSHeaderDXT10);
    output->data = (u8*)alloc(output->width * output->height * 4);

    int blocksX = (output->width  + 3) / 4;
    int blocksY = (output->height + 3) / 4;

    for (int by = 0; by < blocksY; by++) {
        for (int bx = 0; bx < blocksX; bx++) {
            int blockIndex = bx + by * blocksX;
            u8* pBlock = &imageData[blockIndex * PIXELS_PER_BLOCK];
            bc7decomp::unpack_bc7(pBlock, unpackedPixels);

            for (int p = 0; p < PIXELS_PER_BLOCK; p++) {
                int x = ((p % 4) + (bx * 4));
                int y = ((p / 4) + (by * 4));
                int pi = x + (y * output->width);
                int di = pi * 4;

                output->data[di + 0] = unpackedPixels[p].r;
                output->data[di + 1] = unpackedPixels[p].g;
                output->data[di + 2] = unpackedPixels[p].b;
                output->data[di + 3] = unpackedPixels[p].a;
            }
        }
    }
}

bool rgba8_to_dds_bc7_file(const char* filename, RGBA8_Texture* textures, int mip_count = 1) {
    // Check if texture is divisible by 4.
    if (textures[0].width % 4 != 0 || textures[0].height % 4 != 0) {
        LOG_MESSAGE("Texture data cannot be converted to DDS. It must be divisible by 4!\n");
        return false;
    }

    File file = FileOpen(filename, FileMode_Write);

    // Fill header
    DDSHeader header = {0};
    strcpy(header.filecode, "DDS ");
    header.size = 124;
    header.flags = 0x00021007; //DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT
    header.height = textures[0].height;
    header.width = textures[0].width;
    header.mipMapCount = mip_count;
    header.ddspf.size = 32;
    header.ddspf.flags = 0x00000004; // DDPF_FOURCC
    strcpy(header.ddspf.fourCC, "DX10"); //Use extension header
    header.pitchOrLinearSize = ((textures[0].width + 3) / 4) * ((textures[0].height + 3) / 4) * 16;
    header.caps = 0x1000; //DDSCAPS_TEXTURE (contains a texture)

    // Write the header to file
    FileWrite(file, &header, sizeof(header));

    // Write the DX10 header for BC7
    DDSHeaderDXT10 headerDXT10 = {};
    headerDXT10.format = DDSFormat_BC7_UNORM;
    headerDXT10.resourceDimension = 3; // D3D11_RESOURCE_DIMENSION_TEXTURE2D
    headerDXT10.arraySize = 1;
    FileWrite(file, &headerDXT10, sizeof(headerDXT10));

    // Initialize BC7 compression parameters
    bc7enc_compress_block_init();
    bc7enc_compress_block_params params;
    bc7enc_compress_block_params_init(&params);

    // Compress each 4x4 block of the image
    u8 block  [16 * 4];
    u8 pBlock [16 * 4];

    int channels = 4;

    for (int i = 0; i < mip_count; i++) {
        RGBA8_Texture* texture = &textures[i];

        for (int y = 0; y < texture->height; y += 4) {
            for (int x = 0; x < texture->width; x += 4) {
                for (int by = 0; by < 4; ++by) {
                    for (int bx = 0; bx < 4; ++bx) {
                        int srcIndex = ((y + by) * texture->width + (x + bx)) * channels;
                        int dstIndex = (by * 4 + bx) * 4;
                        if ((y + by) < texture->height && (x + bx) < texture->width) {
                            block[dstIndex + 0] = texture->data[srcIndex + 0];
                            block[dstIndex + 1] = texture->data[srcIndex + 1];
                            block[dstIndex + 2] = texture->data[srcIndex + 2];
                            block[dstIndex + 3] = (channels == 4) ? texture->data[srcIndex + 3] : 255;
                        } else {
                            block[dstIndex + 0] = 0;
                            block[dstIndex + 1] = 0;
                            block[dstIndex + 2] = 0;
                            block[dstIndex + 3] = 0;
                        }
                    }
                }

                bc7enc_compress_block(pBlock, block, &params);
                FileWrite(file, pBlock, 16);
            }
        }
    }

    FileClose(file);
    return true;
}

void premultiply_rgba8_texture(RGBA8_Texture* texture) {
    for (int p = 0; p < texture->width * texture->height * 4; p += 4) {
        u8 r = texture->data[p + 0];
        u8 g = texture->data[p + 1];
        u8 b = texture->data[p + 2];
        u8 a = texture->data[p + 3];

        float alpha = a / 255.0f;
        texture->data[p + 0] = (u8)(r * alpha);
        texture->data[p + 1] = (u8)(g * alpha);
        texture->data[p + 2] = (u8)(b * alpha);
    }
}

void generate_mips_for_rgba8(RGBA8_Texture* input, RGBA8_Texture* mips, int mip_levels, AllocFunc alloc) {
    RGBA8_Texture* previous = input;

    for (int i = 0; i < mip_levels - 1; i++) {
        RGBA8_Texture* mip = &mips[i];

        mip->width = previous->width / 2;
        mip->height = previous->height / 2;
        mip->data = (u8*)alloc(mip->width * mip->height * 4);

        for (int y = 0; y < mip->height; ++y) {
            for (int x = 0; x < mip->width; ++x) {
                int sx = x * 2;
                int sy = y * 2;

                int i00 = (sy * previous->width + sx) * 4;
                int i10 = (sy * previous->width + (sx + 1)) * 4;
                int i01 = ((sy + 1) * previous->width + sx) * 4;
                int i11 = ((sy + 1) * previous->width + (sx + 1)) * 4;

                int di = (y * mip->width + x) * 4;

                float a = 0.25f * (u8_to_f01(previous->data[i00+3]) + u8_to_f01(previous->data[i10+3]) +
                                   u8_to_f01(previous->data[i01+3]) + u8_to_f01(previous->data[i11+3]));

                for (int c = 0; c < 3; ++c) {
                    float l = 0.25f * (srgb_to_linear(u8_to_f01(previous->data[i00+c])) +
                                       srgb_to_linear(u8_to_f01(previous->data[i10+c])) +
                                       srgb_to_linear(u8_to_f01(previous->data[i01+c])) +
                                       srgb_to_linear(u8_to_f01(previous->data[i11+c])));

                    mip->data[di + c] = f01_to_u8(linear_to_srgb(l));
                }

                mip->data[di + 3] = f01_to_u8(a);
            }
        }

        previous = mip;
    }
}

void generate_max_mips_for_dds_file(const char* filepath, const char* output, bool force_regen = false) {
    TempArenaMemory tempMemory = TempArenaMemoryBegin(&tempAllocator);
    RGBA8_Texture source = {};

    MemoryBuffer dds = {};
    ReadEntireFileAndNullTerminate(filepath, &dds, TempAllocator);

    DDSHeader header;
    memcpy(&header, dds.buffer, sizeof(DDSHeader));

    RGBA8_Texture* mips = 0;
    int smallest_side = 0;
    int mip_levels = 0;

    if (!is_power_of_two(header.width) || !is_power_of_two(header.height)) {
        printf("Error: Texture must be a power of two: %s\n", filepath);
        goto exit;
    }

    smallest_side = header.width > header.height ? header.height : header.width;
    mip_levels = div2_until_not_divisible_by4_u32(smallest_side);

    if (!force_regen && header.mipMapCount == mip_levels) {
        goto exit;
    }

    load_rgba8_from_dds_bc7(&dds, &source, malloc);

    mips = (RGBA8_Texture*)malloc(mip_levels * sizeof(RGBA8_Texture));
    mips[0] = source;

    generate_mips_for_rgba8(&source, &mips[1], mip_levels, malloc);
    rgba8_to_dds_bc7_file(output, mips, mip_levels);

    for (int i = 0; i < mip_levels; i++) {
        free(mips[i].data);
    }

    printf("Generated %u Mips: %s\n", mip_levels, filepath);

    exit:
        TempArenaMemoryEnd(tempMemory);
}

void generate_padded_rgba8_tilesheet(RGBA8_Texture* input, RGBA8_Texture* output, int padding, int tile_size, AllocFunc alloc) {
    ASSERT_ERROR(input->width  % tile_size == 0, "texture width must be a multiple of tile_size");
    ASSERT_ERROR(input->height % tile_size == 0, "texture height must be a multiple of tile_size");

    int tile_count_x = input->width  / tile_size;
    int tile_count_y = input->height / tile_size;

    output->width  = (tile_count_x * padding * 2) + input->width;
    output->height = (tile_count_y * padding * 2) + input->height;
    output->data = (u8*)alloc(output->width * output->height * 4);

    int padded_tile_size = tile_size + (padding * 2);

    for (int y = 0; y < output->height; y++) {
        int rel_y = (y % padded_tile_size) - padding;
        int clamp_y = (rel_y >= tile_size) ? tile_size-1 : ((rel_y < 0) ? 0 : rel_y);
        int dest_y = clamp_y + ((y / padded_tile_size) * tile_size);

        int source_y_offset = dest_y * input->width;
        int target_y_offset = y * output->width;

        for (int x = 0; x < output->width; x++) {
            int rel_x = (x % padded_tile_size) - padding;
            int clamp_x = (rel_x >= tile_size) ? tile_size-1 : ((rel_x < 0) ? 0 : rel_x);
            int dest_x = clamp_x + ((x / padded_tile_size) * tile_size);

            int source_index = (dest_x + source_y_offset) * 4;
            int target_index = (x + target_y_offset) * 4;

            output->data[target_index + 0] = input->data[source_index + 0];
            output->data[target_index + 1] = input->data[source_index + 1];
            output->data[target_index + 2] = input->data[source_index + 2];
            output->data[target_index + 3] = input->data[source_index + 3];
        }
    }
}

// standard editor process when importing PNGs.
bool process_png_to_dds(const char* input, const char* output, bool premultiply_alpha, int tile_padding = 0) {
    bool processed = false;
    int channels = 0;
    RGBA8_Texture loaded = {};
    RGBA8_Texture padded = {};

    loaded.data = stbi_load(input, &loaded.width, &loaded.height, &channels, 0);

    int smallest_side = 0;
    int mip_levels = 0;
    RGBA8_Texture* mips = 0;

    if (!loaded.data) {
        ASSERT_DEBUG(false, "Error loading image: %s\n", stbi_failure_reason());
        goto clean_up_png;
    }

    if (channels != 4) {
        ASSERT_DEBUG(false, "Error: There must be 4 channels for textures: %s!", input);
        goto clean_up_png;
    }

    if (!is_power_of_two(loaded.width) || !is_power_of_two(loaded.height)) {
        ASSERT_DEBUG(false, "Error: Texture must be a power of two: %s", input);
        goto clean_up_png;
    }

    if (premultiply_alpha) {
        premultiply_rgba8_texture(&loaded);
    }

    generate_padded_rgba8_tilesheet(&loaded, &padded, tile_padding, 256, malloc);

    smallest_side = padded.width > padded.height ? padded.height : padded.width;
    mip_levels = div2_until_not_divisible_by4_u32(smallest_side);

    mips = (RGBA8_Texture*)malloc(mip_levels * sizeof(RGBA8_Texture));
    mips[0] = padded;

    generate_mips_for_rgba8(&padded, &mips[1], mip_levels, malloc);

    if (rgba8_to_dds_bc7_file(output, mips, mip_levels)) {
        processed = true;
        goto clean_up_png;
    }

    clean_up_png:
        stbi_image_free(loaded.data);

    for (int i = 0; i < mip_levels; i++) {
        free(mips[i].data);
    }
    free(mips);

    return processed;
}

// standard editor process when importing PNGs.
void process_dds(const char* input, const char* output) {
    MemoryBuffer dds = {};
    ReadEntireFileAndNullTerminate(input, &dds, TempAllocator);

    DDSHeader header;
    memcpy(&header, dds.buffer, sizeof(DDSHeader));

    int smallest_side = (header.width > header.height) ? header.height : header.width;
    int mip_levels = div2_until_not_divisible_by4_u32(smallest_side);

    if (header.mipMapCount != mip_levels) {
        RGBA8_Texture source = {};
        load_rgba8_from_dds_bc7(&dds, &source, malloc);

        RGBA8_Texture* mips = (RGBA8_Texture*)malloc(mip_levels * sizeof(RGBA8_Texture));
        mips[0] = source;

        generate_mips_for_rgba8(&source, &mips[1], mip_levels, malloc);
        rgba8_to_dds_bc7_file(output, mips, mip_levels);

        for (int i = 0; i < mip_levels; i++) {
            free(mips[i].data);
        }
        free(mips);
    }
}

#endif //_INF_IMAGE_H_