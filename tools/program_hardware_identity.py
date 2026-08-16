#!/usr/bin/env python3

"""Program the AirGradient hardware identity into ESP32-C3 eFuse BLK3.

The script accepts ``indoor`` or ``outdoor``, requires BLK3 to be blank, burns
the corresponding identity, and verifies the result. eFuse writes are
irreversible.

Dependency:
    python3 -m pip install esptool

The installation must provide ``espefuse.py`` on PATH. Use ``--espefuse`` to
specify another executable path.
"""

import argparse
import subprocess
import sys
import tempfile
from pathlib import Path


IDENTITIES = {
    "indoor": {
        "value": 0xA5,
        "bits": (0, 2, 5, 7),
    },
    "outdoor": {
        "value": 0x5A,
        "bits": (1, 3, 4, 6),
    },
}


def run_espefuse(espefuse: str, port: str, *args: str) -> None:
    command = [
        espefuse,
        "--chip",
        "esp32c3",
        "--port",
        port,
        *args,
    ]
    print("+", " ".join(command), flush=True)
    subprocess.run(command, check=True)


def read_user_data(espefuse: str, port: str) -> bytes:
    with tempfile.TemporaryDirectory(prefix="ag-efuse-") as directory:
        dump_path = Path(directory) / "efuse.bin"
        run_espefuse(espefuse, port, "dump", "--file_name", str(dump_path))

        block_path = Path(directory) / "efuse3.bin"
        try:
            data = block_path.read_bytes()
        except OSError as error:
            raise RuntimeError(f"cannot read BLK3 dump: {error}") from error

    if len(data) != 32:
        raise RuntimeError(f"unexpected BLK3 size: {len(data)} bytes")
    return data


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Program the One/Open Air hardware identity in ESP32-C3 BLK3."
    )
    parser.add_argument("identity", choices=IDENTITIES)
    parser.add_argument("--port", default="/dev/ttyACM0")
    parser.add_argument("--espefuse", default="espefuse.py")
    args = parser.parse_args()

    identity = IDENTITIES[args.identity]

    try:
        before = read_user_data(args.espefuse, args.port)
        if any(before):
            print(
                f"ERROR: BLK3 is not blank: {before.hex()}",
                file=sys.stderr,
            )
            return 1

        bits = tuple(str(bit) for bit in identity["bits"])
        run_espefuse(
            args.espefuse,
            args.port,
            "--do-not-confirm",
            "burn_bit",
            "BLOCK_USR_DATA",
            *bits,
        )

        after = read_user_data(args.espefuse, args.port)
        expected = bytes([identity["value"]]) + bytes(31)
        if after != expected:
            print(
                f"ERROR: BLK3 verification failed: expected {expected.hex()}, "
                f"read {after.hex()}",
                file=sys.stderr,
            )
            return 1
    except (OSError, subprocess.CalledProcessError, RuntimeError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1

    print(
        f"Hardware identity programmed: {args.identity} "
        f"(BLK3 byte 0 = 0x{identity['value']:02X})"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
