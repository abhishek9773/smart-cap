#!/usr/bin/env python3
"""Smoke-test the running Smart Cap API; requires a reachable ESP32 device."""

import argparse
import json
import struct
import sys
import urllib.error
import urllib.request


def request(base_url, path, method="GET", body=None, content_type=None):
    headers = {}
    if content_type:
        headers["Content-Type"] = content_type
    data = body
    req = urllib.request.Request(
        base_url.rstrip("/") + path,
        data=data,
        headers=headers,
        method=method,
    )
    try:
        with urllib.request.urlopen(req, timeout=5) as response:
            return response.status, response.headers, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.headers, error.read()


def expect_json(base_url, path, expected_status=200, method="GET", body=None):
    status, _, payload = request(
        base_url,
        path,
        method,
        json.dumps(body).encode() if body is not None else None,
        "application/json" if body is not None else None,
    )
    assert status == expected_status, f"{method} {path}: expected {expected_status}, got {status}: {payload!r}"
    decoded = json.loads(payload)
    assert "ok" in decoded and "data" in decoded and "error" in decoded, f"{path}: missing response envelope"
    return decoded


def pcm_wav_16khz_mono():
    pcm = b"\x00\x00"
    fmt = struct.pack("<HHIIHH", 1, 1, 16000, 32000, 2, 16)
    chunks = b"fmt " + struct.pack("<I", len(fmt)) + fmt
    chunks += b"data" + struct.pack("<I", len(pcm)) + pcm
    return b"RIFF" + struct.pack("<I", len(chunks) + 4) + b"WAVE" + chunks


def run(base_url):
    health = expect_json(base_url, "/health")
    assert health["data"]["service"] == "smart-cap"
    assert isinstance(health["data"]["uptime_ms"], int)

    status = expect_json(base_url, "/api/v1/status")["data"]
    assert "camera" in status and "wifi" in status and "battery" in status
    if not status["battery"]["measurement_available"]:
        assert status["battery"]["voltage_mv"] is None
        assert status["battery"]["percentage"] is None
        assert status["battery"]["charging"] is None

    capabilities = expect_json(base_url, "/api/v1/capabilities")["data"]
    sensors = expect_json(base_url, "/api/v1/sensors")["data"]
    if not capabilities["distance_sensors"]:
        assert sensors["distance_sensors"]["available"] is False
        assert sensors["distance_sensors"]["left_mm"] is None
        assert sensors["distance_sensors"]["right_mm"] is None
    if not capabilities["battery_monitoring"]:
        assert sensors["battery"]["measurement_available"] is False
        assert sensors["battery"]["voltage_mv"] is None
        assert sensors["battery"]["percentage"] is None

    expect_json(base_url, "/api/v1/audio/status")
    expect_json(base_url, "/api/v1/haptics/status")
    expect_json(base_url, "/api/v1/mode")
    expect_json(base_url, "/api/v1/events")
    expect_json(base_url, "/api/v1/metrics")

    expect_json(
        base_url,
        "/api/v1/mode",
        expected_status=400,
        method="POST",
        body={"mode": "invalid"},
    )
    if capabilities["software_mode_control"]:
        original_mode = expect_json(base_url, "/api/v1/mode")["data"]["mode"]
        expect_json(
            base_url,
            "/api/v1/mode",
            method="POST",
            body={"mode": "vibration_only"},
        )
        expect_json(
            base_url,
            "/api/v1/mode",
            method="POST",
            body={"mode": original_mode},
        )
    expect_json(
        base_url,
        "/api/v1/haptics/pattern",
        expected_status=400,
        method="POST",
        body={"pattern": "arbitrary_gpio"},
    )
    expect_json(
        base_url,
        "/api/v1/command",
        expected_status=400,
        method="POST",
        body={"command": "run_shell"},
    )
    expect_json(
        base_url,
        "/api/v1/audio",
        expected_status=415,
        method="POST",
        body=b"not wav",
        content_type="application/octet-stream",
    )
    expect_json(
        base_url,
        "/api/v1/audio",
        expected_status=400,
        method="POST",
        body=b"bad wav",
        content_type="audio/wav",
    )
    status_code, _, response = request(
        base_url,
        "/api/v1/audio",
        method="POST",
        body=b"\x00" * (256 * 1024 + 1),
        content_type="audio/wav",
    )
    assert status_code == 413, f"oversized upload should return 413, got {status_code}: {response[:160]!r}"

    status_code, _, response = request(
        base_url,
        "/api/v1/audio",
        method="POST",
        body=pcm_wav_16khz_mono(),
        content_type="audio/wav",
    )
    assert status_code == 503, f"valid WAV should report unavailable playback hardware, got {status_code}: {response!r}"
    assert json.loads(response)["error"]["code"] == "AUDIO_NOT_CONFIGURED"

    status_code, _, response = request(base_url, "/api/v1/unknown")
    assert status_code == 404, f"unknown route: expected 404, got {status_code}: {response!r}"
    status_code, _, response = request(base_url, "/health", method="POST")
    assert status_code == 405, f"unsupported method should return 405, got {status_code}: {response!r}"

    if status["camera"]["initialized"]:
        status_code, headers, frame = request(base_url, "/api/v1/frame")
        assert status_code == 200, f"camera frame request failed: {status_code}: {frame[:160]!r}"
        assert headers.get_content_type() == "image/jpeg"
        assert frame.startswith(b"\xff\xd8") and frame.endswith(b"\xff\xd9"), "frame body is not a complete JPEG"
        print("PASS: frame endpoint (requires working OV2640 hardware)")
    else:
        status_code, _, response = request(base_url, "/api/v1/frame")
        assert status_code == 503, f"uninitialized camera should return 503, got {status_code}: {response!r}"
        print("SKIP: JPEG contents (camera is not initialized)")

    print("PASS: API envelope, health, status, capabilities, unavailable readings, validation, and error routes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-url", default="http://192.168.4.1")
    args = parser.parse_args()
    try:
        run(args.base_url)
    except (AssertionError, OSError, json.JSONDecodeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
