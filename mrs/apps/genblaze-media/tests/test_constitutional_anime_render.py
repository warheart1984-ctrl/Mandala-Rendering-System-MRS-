"""Tests for constitutional anime render pipeline labeling + stages."""

from __future__ import annotations

import json
from pathlib import Path

import pytest

from app.anime_world_profile import default_example_path, load_anime_world_profile
from app.constitutional_anime_render import (
    BACKEND_CEL_PROXY,
    BACKEND_NONE,
    LANE_BEAUTY,
    LANE_STRUCTURE_ONLY,
    apply_cel_proxy_png,
    build_assertion,
    main,
    run_beauty_stage,
    run_pipeline,
)


def _tiny_png() -> bytes:
    """Minimal valid 2x2 PNG via Pillow."""
    from PIL import Image
    import io

    img = Image.new("RGBA", (8, 8), (180, 140, 120, 255))
    # Gradient so cel-proxy has edges
    for y in range(8):
        for x in range(8):
            img.putpixel((x, y), (40 + x * 20, 60 + y * 15, 90, 255))
    buf = io.BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


def test_build_assertion_shape():
    text = build_assertion(
        profile_id="anime.mandala-cel.v1",
        profile_version="1.0.0",
        structure_source="Engine3D",
        polish_backend="cel-proxy",
        provenance_hash="abcdef0123456789deadbeef",
    )
    assert "AnimeWorldProfile v1.0.0" in text
    assert "Engine3D" in text
    assert "cel-proxy" in text
    assert "abcdef0123456789" in text


def test_structure_only_when_painter_none(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    monkeypatch.delenv("FAL_KEY", raising=False)
    monkeypatch.delenv("FAL_API_KEY", raising=False)
    monkeypatch.delenv("SEEDANCE_API_KEY", raising=False)
    structure_file = tmp_path / "in.png"
    structure_file.write_bytes(_tiny_png())
    out = tmp_path / "out"

    class NS:
        out_dir = str(out)
        profile = str(default_example_path())
        structure = str(structure_file)
        structure_source = "engine3d"
        painter = "none"
        no_cel_proxy = True
        no_reuse_continuity = True
        run_engine3d = False
        intent_id = "intent.test"
        world_id = "world.test"
        timeline_id = "timeline.test"
        probe_only = False

    manifest = run_pipeline(NS())
    assert manifest.lane == LANE_STRUCTURE_ONLY
    assert manifest.polish_backend == BACKEND_NONE
    assert manifest.anime_claim is False
    assert "structure-only" in manifest.assertion or "structure-only" in manifest.lane
    data = json.loads((out / "render-manifest.json").read_text(encoding="utf-8"))
    assert data["anime_claim"] is False
    assert data["path_kind"] == LANE_STRUCTURE_ONLY


def test_cel_proxy_beauty_and_replay(tmp_path: Path):
    profile = load_anime_world_profile(default_example_path())
    structure = _tiny_png()
    beauty, lane, backend, claim, detail = run_beauty_stage(
        structure_png=structure,
        profile=profile,
        painter_pref="cel-proxy",
        allow_cel_proxy=True,
    )
    assert lane == LANE_BEAUTY
    assert backend == BACKEND_CEL_PROXY
    assert claim is True
    assert "cel-proxy" in detail
    replay = apply_cel_proxy_png(structure, profile)
    assert beauty == replay


def test_probe_only_exits_zero(capsys: pytest.CaptureFixture[str]):
    code = main(["--probe-only"])
    assert code == 0
    out = json.loads(capsys.readouterr().out)
    assert out["valid"] is True
    assert "painter_probes" in out
