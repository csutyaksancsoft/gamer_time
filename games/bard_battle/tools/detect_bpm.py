#!/usr/bin/env python3
"""Detect a WAV's fixed BPM and first beat, then update its song.cfg."""

from __future__ import annotations

import argparse
import math
from pathlib import Path
import sys


REPO_AUDIO_DIR = Path(__file__).resolve().parent / "assets" / "audio"


def resolve_wav(value: str) -> Path:
    supplied = Path(value).expanduser()
    candidates = [supplied, REPO_AUDIO_DIR / supplied]
    for candidate in candidates:
        if candidate.is_file():
            return candidate.resolve()
    raise FileNotFoundError(
        f"WAV not found: {value} (also checked {REPO_AUDIO_DIR / supplied})"
    )


def read_config(path: Path) -> tuple[dict[str, str], list[str]]:
    values: dict[str, str] = {}
    comments: list[str] = []
    if not path.exists():
        return values, comments
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#"):
            comments.append(raw_line)
            continue
        key, separator, value = line.partition("=")
        if separator:
            values[key.strip()] = value.strip()
    return values, comments


def normalize_bpm(bpm: float, minimum: float, maximum: float) -> tuple[float, str | None]:
    original = bpm
    while bpm < minimum:
        bpm *= 2.0
    while bpm > maximum:
        bpm /= 2.0
    if math.isclose(original, bpm, rel_tol=1e-9):
        return bpm, None
    return bpm, f"normalized detector result from {original:.3f} BPM to {bpm:.3f} BPM"


def analyze(wav_path: Path, minimum_bpm: float, maximum_bpm: float) -> tuple[float, int, int, float, str | None]:
    try:
        import essentia.standard as es
    except ImportError as error:
        raise RuntimeError(
            "Essentia is required. Install it with: python -m pip install essentia"
        ) from error

    sample_rate = 44100
    audio = es.MonoLoader(filename=str(wav_path), sampleRate=sample_rate)()
    if len(audio) == 0:
        raise RuntimeError(f"Audio file is empty: {wav_path}")

    detector = es.RhythmExtractor2013(
        method="multifeature",
        minTempo=max(40, int(math.floor(minimum_bpm / 2.0))),
        maxTempo=min(250, int(math.ceil(maximum_bpm * 2.0))),
    )
    detected_bpm, ticks, confidence, _estimates, _intervals = detector(audio)
    if detected_bpm <= 0 or len(ticks) == 0:
        raise RuntimeError("No stable beat was detected")

    bpm, normalization = normalize_bpm(float(detected_bpm), minimum_bpm, maximum_bpm)
    first_beat_ms = max(0, int(round(float(ticks[0]) * 1000.0)))
    duration_ms = int(round(len(audio) * 1000.0 / sample_rate))
    return bpm, first_beat_ms, duration_ms, float(confidence), normalization


def write_config(
    config_path: Path,
    wav_path: Path,
    bpm: float,
    first_beat_ms: int,
    duration_ms: int,
) -> None:
    existing, comments = read_config(config_path)
    values = {
        "id": existing.get("id", wav_path.stem),
        "file": wav_path.name,
        "bpm": f"{bpm:.3f}".rstrip("0").rstrip("."),
        "first_beat_ms": str(first_beat_ms),
        "subdivision": existing.get("subdivision", "1"),
        "duration_ms": str(duration_ms),
    }
    header = [line for line in comments if line.strip().startswith("#")]
    output = header + [f"{key}={value}" for key, value in values.items()]
    config_path.write_text("\n".join(output) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Detect fixed BPM/beat offset and update song.cfg beside a WAV."
    )
    parser.add_argument("--wav", required=True, help="WAV path or filename in assets/audio")
    parser.add_argument("--config", help="Output config path; defaults to song.cfg beside the WAV")
    parser.add_argument("--min-bpm", type=float, default=90.0, help="Lowest desired gameplay BPM")
    parser.add_argument("--max-bpm", type=float, default=180.0, help="Highest desired gameplay BPM")
    parser.add_argument("--dry-run", action="store_true", help="Print results without updating song.cfg")
    args = parser.parse_args()

    if args.min_bpm <= 0 or args.max_bpm <= args.min_bpm:
        parser.error("--min-bpm must be positive and lower than --max-bpm")

    try:
        wav_path = resolve_wav(args.wav)
        config_path = Path(args.config).expanduser().resolve() if args.config else wav_path.with_name("song.cfg")
        bpm, offset_ms, duration_ms, confidence, normalization = analyze(
            wav_path, args.min_bpm, args.max_bpm
        )
        print(f"WAV:           {wav_path}")
        print(f"BPM:           {bpm:.3f}")
        print(f"First beat:    {offset_ms} ms")
        print(f"Duration:      {duration_ms} ms")
        print(f"Confidence:    {confidence:.3f}")
        if normalization:
            print(f"Note:          {normalization}")
        if confidence < 1.5:
            print("Warning: low beat confidence; verify BPM and offset in-game.", file=sys.stderr)
        if args.dry_run:
            print("Dry run: song.cfg was not changed.")
        else:
            write_config(config_path, wav_path, bpm, offset_ms, duration_ms)
            print(f"Updated:       {config_path}")
        return 0
    except (FileNotFoundError, RuntimeError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
