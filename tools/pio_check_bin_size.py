"""Fail the build when firmware.bin does not fit the smallest app partition.

PlatformIO's own size check sums ELF sections and leaves out .eh_frame, which
esptool still writes into the image. On arduino-esp32 3.x that hides about
58 KB, so a build can report SUCCESS with an image the bootloader rejects.
"""

import csv
import os

Import("env")


def parse_size(value):
    value = value.strip().upper()
    if value.startswith("0X"):
        return int(value, 16)
    if value.endswith("K"):
        return int(value[:-1], 0) * 1024
    if value.endswith("M"):
        return int(value[:-1], 0) * 1024 * 1024
    return int(value, 0)


def app_partition_size():
    table = env.BoardConfig().get("build.partitions", "partitions.csv")
    table = os.path.join(env.subst("$PROJECT_DIR"), table)
    sizes = []
    with open(table, encoding="utf8") as fp:
        for row in csv.reader(fp):
            if not row or row[0].strip().startswith("#") or len(row) < 5:
                continue
            if row[1].strip() == "app":
                sizes.append(parse_size(row[4]))
    return min(sizes)


def check_bin_size(source, target, env):
    bin_path = target[0].get_abspath()
    image = os.path.getsize(bin_path)
    limit = app_partition_size()
    percent = 100.0 * image / limit
    print("Image: %d bytes of %d app partition (%.1f%%)" % (image, limit, percent))
    if image > limit:
        print("Error: firmware.bin is %d bytes too large for the app partition"
              % (image - limit))
        env.Exit(1)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", check_bin_size)
