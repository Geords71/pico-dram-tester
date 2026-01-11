import warnings
warnings.filterwarnings("ignore", category=UserWarning, module="fs")
import click
import os
from os import path
from pyfatfs.PyFat import PyFat
import fs
from fs.osfs import OSFS
import string

IMAGE_SIZE_BYTES = 1024 * 64

class FatImageGenerator:
    def __init__(self, input_dir, output_file):
        return

@click.command()
@click.option("--input-dir", type=str, help="Absolute path to input directory.")
@click.option("--output-file", type=str, help="Absolute path to output header file.")
def generate_fat_header(input_dir, output_file):
    output_dir = path.dirname(output_file)
    split = os.path.splitext(output_file)
    name = os.path.basename(split[0])
    image_path = ".".join([split[0], "raw"])
    create_image(input_dir, image_path)
    add_files_to_image(input_dir, image_path)
    convert_image_to_header(image_path, output_file, name)
    return

def create_image(input_dir, image_path):
    with open(image_path, "wb") as f:
        f.write(b"\x00" * IMAGE_SIZE_BYTES)

    # Initialize FAT12 filesystem
    pf = PyFat()
    pf.mkfs(image_path, label="PICO_FS", fat_type=12)
    pf.close()

def add_files_to_image(input_dir, image_path):
    # Walk the source directory tree
    for root, dirs, files in os.walk(input_dir):
        rel_root = path.relpath(root, input_dir)
        if rel_root == ".":
            rel_root = ""
        # Create directories
        if rel_root:
            try:
                with fs.open_fs(f"fat://{image_path}") as fat_fs:
                    with fat_fs.open(dest_file, "wb") as f_img:
                        fat_fs.makedir(rel_root)
            except FileExistsError:
                pass

        # Copy files
        for fname in files:
            src_file = path.join(root, fname)
            dest_file = path.join(rel_root, fname) if rel_root else fname
            with open(src_file, "rb") as sf:
                data = sf.read()
                with fs.open_fs(f"fat://{image_path}") as fat_fs:
                    with fat_fs.open(dest_file, "wb") as f_img:
                        f_img.write(data)
    print(f"FAT12 image created at {image_path}")

def convert_image_to_header(image_path, output_file, name):
    with open(image_path, "rb") as f:
        data = f.read()

    with open(output_file, "w") as f:
        f.write(f"#ifndef {name.upper()}_H\n#define {name.upper()}_H\n\n")
        f.write(f"#include <stdint.h>\n\n")
        f.write(f"const uint8_t {name}_data[{len(data)}] = {{\n")
        
        for i, byte in enumerate(data):
            if i % 16 == 0:  # New line every 16 bytes for readability
                f.write(f"/* {i:08x} */  ")
            char = chr(byte)
            if 31 < byte < 127 and byte not in [92, 39]:
                f.write(f"'{char}',  ")
            else:
                f.write(f"0x{byte:02x}, ")
            if (i + 1) % 16 == 0:  # New line every 16 bytes for readability
                f.write(f"\n")
                
        f.write(f"\n}};\n\nunsigned int {name}_len = {len(data)};\n")
        f.write("#endif")
    print(f"FAT12 image header created at {output_file}")

if __name__ == "__main__":
    generate_fat_header()