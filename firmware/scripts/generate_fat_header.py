#!/usr/bin/env python3
import os
import struct
import click

# 512 KB FAT12 volume + MBR
BYTES_PER_SECTOR    = 512

# FAT12 volume size (boot sector + FATs + root + data)
TOTAL_SECTORS       = 1024      # 1024 * 512 = 512 KB

# Add 1 sector for MBR at LBA0
SECTORS_WITH_MBR    = TOTAL_SECTORS + 1

SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS    = 1         # boot sector at LBA1
NUM_FATS            = 2
ROOT_ENTRIES        = 64
SECTORS_PER_FAT     = 3

# Non-floppy geometry so Windows treats it as a normal USB disk
SECTORS_PER_TRACK   = 32
NUM_HEADS           = 4
MEDIA               = 0xF8      # hard-disk type, not floppy


def to_83(name: str) -> bytes:
    base, ext = os.path.splitext(name)
    ext = ext[1:] if ext.startswith(".") else ext

    base = "".join(c for c in base.upper() if c.isalnum())[:8]
    ext  = "".join(c for c in ext.upper()  if c.isalnum())[:3]

    if not base:
        base = "NONAME"
    return f"{base:<8}{ext:<3}".encode("ascii")


def mk_mbr() -> bytearray:
    mbr = bytearray(512)

    # Single partition entry at offset 446
    part = bytearray(16)
    part[0] = 0x00                     # boot flag
    part[1:4] = b"\x00\x02\x00"        # CHS start (ignored by Windows for USB)
    part[4] = 0x01                     # type = FAT12
    part[5:8] = b"\x00\x00\x00"        # CHS end (ignored)
    struct.pack_into("<I", part, 8, 1)             # LBA start = 1 (boot sector)
    struct.pack_into("<I", part, 12, TOTAL_SECTORS) # sector count = 1024

    mbr[446:446+16] = part
    mbr[510] = 0x55
    mbr[511] = 0xAA
    return mbr


def mk_boot_sector() -> bytearray:
    bs = bytearray(512)
    bs[0:3] = b'\xEB\x3C\x90'
    bs[3:11] = b"MSDOS5.0"

    struct.pack_into("<H", bs, 11, BYTES_PER_SECTOR)
    bs[13] = SECTORS_PER_CLUSTER
    struct.pack_into("<H", bs, 14, RESERVED_SECTORS)
    bs[16] = NUM_FATS
    struct.pack_into("<H", bs, 17, ROOT_ENTRIES)
    struct.pack_into("<H", bs, 19, TOTAL_SECTORS)
    bs[21] = MEDIA
    struct.pack_into("<H", bs, 22, SECTORS_PER_FAT)
    struct.pack_into("<H", bs, 24, SECTORS_PER_TRACK)
    struct.pack_into("<H", bs, 26, NUM_HEADS)
    struct.pack_into("<I", bs, 28, 1)  # hidden sectors = 1 (MBR at LBA0)
    struct.pack_into("<I", bs, 32, 0)  # large total sectors (unused for FAT12)

    # Drive number, reserved, boot signature, volume ID, label, fs type
    bs[36] = 0x00
    bs[37] = 0x00
    bs[38] = 0x29
    struct.pack_into("<I", bs, 39, 0x12345678)
    bs[43:54] = b"NO NAME    "
    bs[54:62] = b"FAT12   "

    bs[510] = 0x55
    bs[511] = 0xAA
    return bs


def fat12_set(fat: bytearray, index: int, value: int) -> None:
    # value is 12-bit
    byte_index = (index * 3) // 2
    if index & 1:
        # odd cluster
        fat[byte_index]   = (fat[byte_index] & 0x0F) | ((value << 4) & 0xF0)
        fat[byte_index+1] = (value >> 4) & 0xFF
    else:
        # even cluster
        fat[byte_index]   = value & 0xFF
        fat[byte_index+1] = (fat[byte_index+1] & 0xF0) | ((value >> 8) & 0x0F)


def write_c_header(path: str, img: bytes, name: str) -> None:
    with open(path, "w", newline="\n") as f:
        length = len(img)
        f.write("#ifndef FAT12_IMAGE_H\n")
        f.write("#define FAT12_IMAGE_H\n")
        f.write("#include \"pico.h\"\n\n")
        f.write(f"// Generated FAT12 image ({length} bytes)\n")
        f.write(f"const unsigned int {name}_len = {length};\n")
        f.write(f"const uint8_t __in_flash(\"fat_data\") {name}_data[{length}] = {{\n")

        for i in range(0, len(img), 16):
            chunk = img[i:i+16]
            hexes = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"    {hexes},\n")

        f.write("};\n\n#endif // FAT12_IMAGE_H\n")


def build_image(src_dir: str, output_header: str) -> None:
    split = os.path.splitext(output_header)
    name = os.path.basename(split[0])
    image_path = split[0] + ".img"

    # Allocate full image: MBR + FAT12 volume
    img = bytearray(BYTES_PER_SECTOR * SECTORS_WITH_MBR)

    # LBA0 = MBR
    img[0:512] = mk_mbr()

    # LBA1 = FAT12 boot sector
    img[512:1024] = mk_boot_sector()

    # Layout within the full image (absolute offsets)
    # Volume starts at LBA1
    root_dir_sectors = ((ROOT_ENTRIES * 32) + (BYTES_PER_SECTOR - 1)) // BYTES_PER_SECTOR

    fat1_off = (1 + RESERVED_SECTORS) * BYTES_PER_SECTOR
    fat2_off = fat1_off + SECTORS_PER_FAT * BYTES_PER_SECTOR
    root_off = (1 + RESERVED_SECTORS + NUM_FATS * SECTORS_PER_FAT) * BYTES_PER_SECTOR
    data_off = root_off + root_dir_sectors * BYTES_PER_SECTOR

    # For this geometry:
    # LBA1  = boot
    # LBA2-4  = FAT1 (3 sectors)
    # LBA5-7  = FAT2 (3 sectors)
    # LBA8-11 = root (4 sectors)
    # LBA12+  = data
    assert data_off == 12 * BYTES_PER_SECTOR
    assert data_off <= len(img)

    # FAT buffer (one FAT; mirrored into FAT2)
    fat = bytearray(SECTORS_PER_FAT * BYTES_PER_SECTOR)
    # Reserved clusters 0 and 1
    fat[0] = MEDIA
    fat[1] = 0xFF
    fat[2] = 0xFF

    next_free_cluster = 2
    cluster_size = BYTES_PER_SECTOR * SECTORS_PER_CLUSTER
    root_ptr = root_off

    # Files
    for entry in sorted(os.listdir(src_dir)):
        path = os.path.join(src_dir, entry)
        if not os.path.isfile(path):
            continue

        with open(path, "rb") as fh:
            data = fh.read()
        size = len(data)

        clusters = []
        pos = 0
        while pos < size:
            c = next_free_cluster
            next_free_cluster += 1
            clusters.append(c)
            pos += cluster_size

        if not clusters:
            continue

        print(f"FILE {entry}: size={size}, clusters={clusters}")
        max_clusters = (SECTORS_PER_FAT * BYTES_PER_SECTOR * 2) // 3  # FAT12 entries
        assert next_free_cluster < max_clusters, "FAT full: too many files/data"

        # FAT chain
        for i, c in enumerate(clusters):
            if i == len(clusters) - 1:
                fat12_set(fat, c, 0xFFF)
            else:
                fat12_set(fat, c, clusters[i+1])

        # Data
        pos = 0
        for c in clusters:
            off = data_off + (c - 2) * cluster_size
            chunk = data[pos:pos+cluster_size]
            if len(chunk) < cluster_size:
                chunk = chunk + bytes(cluster_size - len(chunk))
            img[off:off+cluster_size] = chunk
            pos += cluster_size

        # Directory entry
        name83 = to_83(entry)
        ent = bytearray(32)
        ent[0:11] = name83
        ent[11] = 0x20  # archive
        struct.pack_into("<H", ent, 26, clusters[0])
        struct.pack_into("<I", ent, 28, size)
        img[root_ptr:root_ptr+32] = ent
        root_ptr += 32

    # Mirror FAT into both copies
    img[fat1_off:fat1_off+len(fat)] = fat
    img[fat2_off:fat2_off+len(fat)] = fat

    # We need an exact multiple of 4k!
    pad = (-len(img)) % 4096
    img += bytes(pad)

    with open(image_path, "wb") as f:
        f.write(img)

    write_c_header(output_header, img, name)
    print(f"Wrote C header {output_header} with FAT12+MBR image ({len(img)} bytes)")


@click.command()
@click.option("--input-dir", type=str, required=True,
              help="Absolute path to input directory.")
@click.option("--output-file", type=str, required=True,
              help="Absolute path to output header file.")
def cli(input_dir, output_file):
    build_image(input_dir, output_file)


if __name__ == "__main__":
    cli()
