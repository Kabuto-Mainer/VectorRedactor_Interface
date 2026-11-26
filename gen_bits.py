#!/usr/bin/env python3
"""Генерация числа заданной битовой длины с заданным префиксом.

Префикс можно задавать двоичным ('1010', '0b1010') или шестнадцатеричным
('0x1f', 'ff'). Старшие биты числа фиксируются префиксом. Следующие биты —
это тег пользователя, вычисленный из логина, имени хоста и machine-id.
Младшие биты заполняются случайно.

Тег занимает отдельные биты сразу после префикса, поэтому пользователи с
разными тегами гарантированно получают разные числа. Совпадение возможно
только у пользователей с одинаковым тегом; вероятность зависит от того,
сколько бит отдано под тег (--tag-bits).

Примеры:
    ./gen_bits.py 8 --prefix 101
    ./gen_bits.py 16 --prefix 0b1010 -n 5 --format hex
    ./gen_bits.py 16 --prefix 0x1f -n 5 --format bin
    ./gen_bits.py 32 --prefix 111 --seed 42 --format bin
    ./gen_bits.py 32 --prefix 111 --identity alice
"""

from __future__ import annotations

import argparse
import getpass
import hashlib
import os
import random
import secrets
import socket
import sys
from pathlib import Path

MACHINE_ID_PATHS = ("/etc/machine-id", "/var/lib/dbus/machine-id")
UINT64_MASK = (1 << 64) - 1
HEX_DIGITS = "0123456789abcdef"


def parse_prefix(value: str) -> str:
    """Принимает '1010', '0b1010', '0x1f' или 'ff'; возвращает биты.

    Строка только из '0' и '1' без префикса считается двоичной, строка
    с другими hex-цифрами — шестнадцатеричной. Чтобы задать hex-префикс
    из одних нулей и единиц, используйте '0x' (например, '0x10').

    Каждая hex-цифра занимает 4 бита, поэтому ведущие нули значимы:
    '0xf' — это 4 бита '1111', а '0x0f' — 8 бит '00001111'.
    """
    value = value.strip().replace("_", "")
    lowered = value.lower()

    if lowered.startswith("0b"):
        digits = value[2:]
        if not digits:
            raise argparse.ArgumentTypeError("префикс не может быть пустым")
        if any(ch not in "01" for ch in digits):
            raise argparse.ArgumentTypeError(
                "двоичный префикс должен состоять только из символов '0' и '1'"
            )
        return digits

    if lowered.startswith("0x"):
        digits = value[2:]
        if not digits:
            raise argparse.ArgumentTypeError("hex-префикс не может быть пустым")
        if any(ch not in HEX_DIGITS for ch in digits.lower()):
            raise argparse.ArgumentTypeError(
                "hex-префикс должен состоять только из символов 0-9, a-f"
            )
        return format(int(digits, 16), f"0{4 * len(digits)}b")

    if not value:
        raise argparse.ArgumentTypeError("префикс не может быть пустым")

    if all(ch in "01" for ch in value):
        return value

    if all(ch in HEX_DIGITS for ch in lowered):
        return format(int(value, 16), f"0{4 * len(value)}b")

    raise argparse.ArgumentTypeError(
        "префикс должен быть двоичным (например, 1010 или 0b1010) "
        "или шестнадцатеричным (например, 0x1f или ff)"
    )


def read_machine_id() -> str:
    """Прочитать machine-id, если он доступен."""
    for path in MACHINE_ID_PATHS:
        try:
            value = Path(path).read_text(encoding="utf-8").strip()
        except OSError:
            continue
        if value:
            return value
    return ""


def get_identity() -> str:
    """Стабильная идентичность: логин + UID + хост + machine-id."""
    try:
        uid = str(os.getuid())
    except AttributeError:  # Windows
        uid = ""
    return "\0".join(
        (getpass.getuser(), uid, socket.gethostname(), read_machine_id())
    )


def identity_fingerprint(identity: str) -> int:
    """64-битный отпечаток идентичности."""
    digest = hashlib.blake2b(identity.encode("utf-8"), digest_size=8).digest()
    return int.from_bytes(digest, "big")


def choose_layout(
    bits: int, prefix_len: int, tag_bits: int | None
) -> tuple[int, int]:
    """Вернуть (tag_bits, random_bits) для свободной части числа."""
    free_bits = bits - prefix_len
    if free_bits < 0:
        raise ValueError("длина префикса больше числа битов")

    if free_bits == 0:
        if tag_bits not in (None, 0):
            raise ValueError("под тег пользователя не осталось свободных битов")
        return 0, 0

    if tag_bits is None:
        # Половина свободных битов — тегу, половина — случайному хвосту.
        tag_bits = max(1, free_bits // 2)

    if not 0 <= tag_bits <= free_bits:
        raise ValueError(
            f"количество битов тега ({tag_bits}) должно быть "
            f"в диапазоне 0..{free_bits}"
        )

    return tag_bits, free_bits - tag_bits


def generate(
    bits: int,
    prefix: str,
    tag_bits: int,
    fingerprint: int,
    rng: random.Random | None = None,
) -> int:
    """Собрать число: префикс | тег пользователя | случайные биты."""
    free_bits = bits - len(prefix)
    random_bits = free_bits - tag_bits

    value = (int(prefix, 2) if prefix else 0) << free_bits

    if tag_bits:
        tag = fingerprint >> (64 - tag_bits)
        value |= tag << random_bits

    if random_bits:
        randbits = rng.getrandbits if rng is not None else secrets.randbits
        value |= randbits(random_bits)

    return value


def format_number(number: int, bits: int, fmt: str) -> str:
    if fmt == "bin":
        return f"0b{number:0{bits}b}"
    if fmt == "hex":
        width = (bits + 3) // 4
        return f"0x{number:0{width}x}"
    return str(number)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Сгенерировать число заданного количества битов "
            "с заданным двоичным или hex-префиксом."
        ),
        epilog=(
            "Формат числа: <префикс> | <тег пользователя> | <случайные биты>. "
            "Пользователи с разными тегами не могут получить одинаковые числа."
        ),
    )
    parser.add_argument("bits", type=int, help="количество битов (1..64)")
    parser.add_argument(
        "-p",
        "--prefix",
        type=parse_prefix,
        default=None,
        help=(
            "префикс: двоичный (1010 или 0b1010) или hex (0x1f, ff); "
            "по умолчанию пустой"
        ),
    )
    parser.add_argument(
        "-n",
        "--count",
        type=int,
        default=1,
        help="сколько чисел сгенерировать (по умолчанию 1)",
    )
    parser.add_argument(
        "-f",
        "--format",
        choices=("dec", "hex", "bin"),
        default="dec",
        help="формат вывода (по умолчанию dec)",
    )
    parser.add_argument(
        "-s",
        "--seed",
        type=int,
        default=None,
        help=(
            "seed для воспроизводимой генерации; смешивается с ID "
            "пользователя, поэтому у разных пользователей результаты разные"
        ),
    )
    parser.add_argument(
        "--tag-bits",
        type=int,
        default=None,
        help=(
            "сколько свободных битов отдать под тег пользователя "
            "(по умолчанию половина; больше битов — меньше шанс совпадения "
            "между пользователями, меньше — больше случайности)"
        ),
    )
    parser.add_argument(
        "--identity",
        default=None,
        help=(
            "переопределить идентичность пользователя "
            "(по умолчанию логин + хост + machine-id)"
        ),
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    prefix = args.prefix or ""

    if not 1 <= args.bits <= 64:
        parser.error("количество битов должно быть в диапазоне 1..64")
    if len(prefix) > args.bits:
        parser.error(
            f"длина префикса ({len(prefix)}) больше числа битов ({args.bits})"
        )
    if args.count < 1:
        parser.error("количество чисел должно быть >= 1")

    try:
        tag_bits, _ = choose_layout(args.bits, len(prefix), args.tag_bits)
    except ValueError as exc:
        parser.error(str(exc))

    free_bits = args.bits - len(prefix)
    if free_bits == 0:
        print(
            "warning: префикс занимает все биты — результат полностью определён",
            file=sys.stderr,
        )
    elif tag_bits == 0:
        print(
            "warning: тег пользователя отключён — уникальность между "
            "пользователями не гарантируется",
            file=sys.stderr,
        )

    identity = args.identity if args.identity is not None else get_identity()
    fingerprint = identity_fingerprint(identity)

    if args.seed is None:
        rng = None
    else:
        # Даже одинаковый --seed у разных пользователей даёт разные числа.
        rng = random.Random((args.seed & UINT64_MASK) ^ fingerprint)

    for _ in range(args.count):
        number = generate(args.bits, prefix, tag_bits, fingerprint, rng)
        print(format_number(number, args.bits, args.format))

    return 0


if __name__ == "__main__":
    sys.exit(main())
