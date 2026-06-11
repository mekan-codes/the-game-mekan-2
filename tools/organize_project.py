from __future__ import annotations

import os
import re
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DRY_RUN = "--dry-run" in os.sys.argv

SOURCE_PACK_DIRS = [
    "crane_operator_asset_pack_v11_polished",
    "crane_operator_monster_asset_pack_v12",
    "crane_operator_ready_import_pack_v15_fixed_alpha",
    "crane_operator_ready_import_pack_v14",
    "crane_operator_animation_pack_v13",
    "crane_operator_asset_pack_v10_clean",
    "crane_operator_individual_asset_pack_v9",
]

OLD_PATCH_SCRIPTS = [
    "apply_game_fixes.py",
    "apply_boss_difficulty_patch.py",
    "apply_timing_hazard_campaign.py",
    "apply_hazard_visual_bugfix.py",
]

JUNK_PATTERNS = [
    "imagegen.png",
    "game-bug.mp4",
    "asset_contact.jpg",
]

GENERATED_DIRS = [
    "game_bug_frames",
    "__pycache__",
]

ZIP_PATTERNS = [
    "crane_operator_*pack*.zip",
    "Assignment9.zip",
    "Assignment10.zip",
]


def rel(path: Path) -> str:
    try:
        return path.relative_to(ROOT).as_posix()
    except ValueError:
        return path.as_posix()


def log(message: str) -> None:
    print(message)


def ensure_dir(path: Path) -> None:
    if not DRY_RUN:
        path.mkdir(parents=True, exist_ok=True)


def copy_file(src: Path, dst: Path) -> bool:
    if not src.exists() or not src.is_file():
        return False
    if dst.exists() and dst.stat().st_size == src.stat().st_size:
        return False
    log(f"COPY {rel(src)} -> {rel(dst)}")
    ensure_dir(dst.parent)
    if not DRY_RUN:
        shutil.copy2(src, dst)
    return True


def move_file(src: Path, dst: Path) -> bool:
    if not src.exists() or not src.is_file():
        return False
    if src.resolve() == dst.resolve():
        return False
    log(f"MOVE {rel(src)} -> {rel(dst)}")
    ensure_dir(dst.parent)
    if not DRY_RUN:
        if dst.exists():
            dst.unlink()
        shutil.move(str(src), str(dst))
    return True


def delete_path(path: Path) -> bool:
    if not path.exists():
        return False
    log(f"DELETE {rel(path)}")
    if not DRY_RUN:
        if path.is_dir():
            shutil.rmtree(path)
        else:
            path.unlink()
    return True


def clean_empty_dirs(path: Path) -> None:
    if not path.exists() or not path.is_dir():
        return
    for child in list(path.iterdir()):
        if child.is_dir():
            clean_empty_dirs(child)
    if path != ROOT:
        try:
            if not any(path.iterdir()):
                log(f"RMDIR {rel(path)}")
                if not DRY_RUN:
                    path.rmdir()
        except OSError:
            pass


def copy_tree_files(src_dir: Path, dst_dir: Path) -> int:
    count = 0
    if not src_dir.exists():
        return 0
    for src in src_dir.rglob("*"):
        if src.is_file():
            dst = dst_dir / src.relative_to(src_dir)
            if copy_file(src, dst):
                count += 1
    return count


def collect_assets() -> None:
    # v11 polished pack: already has good world/background/effects/ui grouping.
    v11 = ROOT / "crane_operator_asset_pack_v11_polished" / "assets"
    copy_tree_files(v11 / "world", ROOT / "assets" / "world")
    copy_tree_files(v11 / "backgrounds", ROOT / "assets" / "backgrounds")
    copy_tree_files(v11 / "effects", ROOT / "assets" / "effects")
    copy_tree_files(v11 / "ui", ROOT / "assets" / "ui")

    # v12 monster/hazard static sprites: put in a real hazards folder.
    v12_assets = ROOT / "crane_operator_monster_asset_pack_v12" / "assets"
    if v12_assets.exists():
        for png in v12_assets.glob("*.png"):
            copy_file(png, ROOT / "assets" / "hazards" / png.name)

    # v15 cleaned animation frames: keep only cleaned frames under assets/animations.
    v15_frames = ROOT / "crane_operator_ready_import_pack_v15_fixed_alpha" / "assets" / "frames_clean"
    copy_tree_files(v15_frames, ROOT / "assets" / "animations")

    # Fonts: keep a root font for simple TTF loading, and a copy in assets/fonts.
    font_names = ["DejaVuSansMono.ttf", "consola.ttf", "lucon.ttf"]
    for folder_name in ["Assignment9", "Assignment10"]:
        folder = ROOT / folder_name
        for name in font_names:
            src = folder / name
            if src.exists():
                copy_file(src, ROOT / "assets" / "fonts" / src.name)
                if src.name == "DejaVuSansMono.ttf":
                    copy_file(src, ROOT / "DejaVuSansMono.ttf")
                break

    # Runtime DLLs from old assignment folders are no longer a reason to keep the whole folders.
    for folder_name in ["Assignment9", "Assignment10"]:
        folder = ROOT / folder_name
        for dll in ["SDL2.dll", "SDL2_ttf.dll", "SDL2_image.dll"]:
            copy_file(folder / dll, ROOT / "runtime" / dll)


def patch_crane_game_paths() -> None:
    path = ROOT / "crane_game.c"
    if not path.exists():
        log("WARN crane_game.c not found; skipping code path patch")
        return
    text = path.read_text(encoding="utf-8")
    original = text

    # Simplify asset lookup: root assets first, then old source-pack fallback only while cleaning.
    text = re.sub(
        r'snprintf\(fullPath, sizeof\(fullPath\), "crane_operator_asset_pack_v11_polished/%s", relativePath\);\s*texture = IMG_LoadTexture\(app->renderer, fullPath\);\s*if \(texture != NULL\) \{\s*SDL_SetTextureBlendMode\(texture, SDL_BLENDMODE_BLEND\);\s*app->assets.loadedCount\+\+;\s*return texture;\s*\}\s*snprintf\(fullPath, sizeof\(fullPath\), "crane_operator_monster_asset_pack_v12/%s", relativePath\);\s*texture = IMG_LoadTexture\(app->renderer, fullPath\);\s*if \(texture != NULL\) \{\s*SDL_SetTextureBlendMode\(texture, SDL_BLENDMODE_BLEND\);\s*app->assets.loadedCount\+\+;\s*return texture;\s*\}',
        'snprintf(fullPath, sizeof(fullPath), "assets/%s", relativePath);\n'
        '    texture = IMG_LoadTexture(app->renderer, fullPath);\n'
        '    if (texture != NULL) {\n'
        '        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);\n'
        '        app->assets.loadedCount++;\n'
        '        return texture;\n'
        '    }',
        text,
        flags=re.S,
    )

    # Because load_assets() passes "assets/...", the assets/%s fallback would become assets/assets/...
    # Keep the direct relative path as canonical and use no pack fallback after organization.
    text = text.replace('snprintf(fullPath, sizeof(fullPath), "assets/%s", relativePath);\n'
                        '    texture = IMG_LoadTexture(app->renderer, fullPath);\n'
                        '    if (texture != NULL) {\n'
                        '        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);\n'
                        '        app->assets.loadedCount++;\n'
                        '        return texture;\n'
                        '    }',
                        '/* Assets are expected directly under the root assets/ folder after organization. */')

    # Font paths should no longer depend on Assignment9/Assignment10 folders.
    text = text.replace('        "Assignment9/DejaVuSansMono.ttf",\n        "Assignment10/DejaVuSansMono.ttf",',
                        '        "assets/fonts/DejaVuSansMono.ttf",')

    # Static hazard sprites now live in assets/hazards/.
    replacements = {
        '"assets/01_magnet_tower.png"': '"assets/hazards/01_magnet_tower.png"',
        '"assets/02_swing_hammer_rig.png"': '"assets/hazards/02_swing_hammer_rig.png"',
        '"assets/03_twin_turbine_fan.png"': '"assets/hazards/03_twin_turbine_fan.png"',
        '"assets/04_moving_dock_heavy.png"': '"assets/hazards/04_moving_dock_heavy.png"',
        '"assets/05_heavy_hazard_container.png"': '"assets/hazards/05_heavy_hazard_container.png"',
        '"assets/06_precision_fragile_cargo.png"': '"assets/hazards/06_precision_fragile_cargo.png"',
        '"assets/07_hydraulic_crusher.png"': '"assets/hazards/07_hydraulic_crusher.png"',
        '"assets/08_extendable_bridge_platform.png"': '"assets/hazards/08_extendable_bridge_platform.png"',
        '"assets/09_laser_security_gate.png"': '"assets/hazards/09_laser_security_gate.png"',
        '"assets/10_rotating_sweeper_arm.png"': '"assets/hazards/10_rotating_sweeper_arm.png"',
    }
    for old, new in replacements.items():
        text = text.replace(old, new)

    # Animation frames now live in assets/animations/<folder>/<basename>_NN.png.
    text = text.replace(
        'snprintf(path, sizeof(path), "crane_operator_ready_import_pack_v15_fixed_alpha/assets/frames_clean/%s/%s_%02d.png",\n                 folder, basename, i);',
        'snprintf(path, sizeof(path), "assets/animations/%s/%s_%02d.png",\n                 folder, basename, i);'
    )
    text = re.sub(
        r'\s*if \(sprite->frames\[i\] == NULL\) \{\s*snprintf\(fallbackPath, sizeof\(fallbackPath\),\s*"crane_operator_ready_import_pack_v15_fixed_alpha/crane_operator_ready_import_pack_v15_fixed_alpha/assets/frames_clean/%s/%s_%02d.png",\s*folder, basename, i\);\s*sprite->frames\[i\] = IMG_LoadTexture\(app->renderer, fallbackPath\);\s*if \(sprite->frames\[i\] == NULL\) \{\s*printf\("Animation frame missing or failed to load: %s or %s \(%s\)\\n",\s*path, fallbackPath, IMG_GetError\(\)\);',
        '\n        if (sprite->frames[i] == NULL) {\n            printf("Animation frame missing or failed to load: %s (%s)\\n",\n                   path, IMG_GetError());',
        text,
        flags=re.S,
    )
    text = text.replace('    char fallbackPath[512];\n', '')

    if text != original:
        log("PATCH crane_game.c asset/font paths")
        if not DRY_RUN:
            path.write_text(text, encoding="utf-8")


def patch_build_script() -> None:
    path = ROOT / "build_crane_game.bat"
    if not path.exists():
        log("WARN build_crane_game.bat not found; skipping build script patch")
        return
    text = path.read_text(encoding="utf-8")
    original = text
    text = text.replace('echo SDL2_image: found, PNG v11/v12 assets enabled',
                        'echo SDL2_image: found, organized PNG assets enabled')
    text = text.replace(
        '    rem The assignment folders include runtime DLLs but not headers/libs.\n'
        '    rem Copy those DLLs beside the EXE when available so the game can launch directly.\n'
        '    if exist "Assignment9\\SDL2.dll" copy /Y "Assignment9\\SDL2.dll" "SDL2.dll" >nul\n'
        '    if exist "Assignment9\\SDL2_ttf.dll" copy /Y "Assignment9\\SDL2_ttf.dll" "SDL2_ttf.dll" >nul\n',
        '    rem Copy runtime DLLs beside the EXE when available so the game can launch directly.\n'
        '    if exist "runtime\\SDL2.dll" copy /Y "runtime\\SDL2.dll" "SDL2.dll" >nul\n'
        '    if exist "runtime\\SDL2_ttf.dll" copy /Y "runtime\\SDL2_ttf.dll" "SDL2_ttf.dll" >nul\n'
    )
    if text != original:
        log("PATCH build_crane_game.bat runtime paths")
        if not DRY_RUN:
            path.write_text(text, encoding="utf-8")


def write_repo_docs() -> None:
    docs_dir = ROOT / "docs"
    ensure_dir(docs_dir)
    layout = docs_dir / "PROJECT_STRUCTURE.txt"
    content = """Crane Operator: Safe Cargo Delivery - Project Structure\n\nRoot files:\n  crane_game.c              Main SDL2 C source code.\n  build_crane_game.bat      Windows build script.\n  crane_game.exe            Built game executable, produced by the build script.\n  README.txt                Game instructions and build/run notes.\n  DejaVuSansMono.ttf        Local font fallback for SDL_ttf.\n\nFolders:\n  assets/backgrounds/       Background images.\n  assets/world/             Main world/gameplay sprites.\n  assets/effects/           Wind/effect helper sprites.\n  assets/ui/                Menu/HUD/progress UI assets.\n  assets/hazards/           Static hazard sprites.\n  assets/animations/        Optional cleaned animation frames.\n  assets/fonts/             Font backup.\n  runtime/                  Optional SDL runtime DLL fallback.\n  tools/                    Repo maintenance scripts.\n  docs/                     Notes/previews/documentation.\n\nRemoved/cleaned:\n  Assignment9/ and Assignment10/ are removed after extracting any needed font/DLL files.\n  Old asset-pack folders/zips are removed after copying required assets into assets/.\n  Old one-off patch scripts are removed after the final game state is stable.\n"""
    if not layout.exists() or layout.read_text(encoding="utf-8") != content:
        log("WRITE docs/PROJECT_STRUCTURE.txt")
        if not DRY_RUN:
            layout.write_text(content, encoding="utf-8")


def cleanup_old_files() -> None:
    # Old patches are implementation history, not part of the final school project.
    for name in OLD_PATCH_SCRIPTS:
        delete_path(ROOT / name)

    # Delete generated screenshots/videos/contact sheets that are not used by the game.
    for name in JUNK_PATTERNS:
        delete_path(ROOT / name)
    for folder in GENERATED_DIRS:
        delete_path(ROOT / folder)

    # Delete old source asset folders after assets have been collected.
    for folder in SOURCE_PACK_DIRS:
        delete_path(ROOT / folder)

    # Delete old zip uploads/source packs.
    for pattern in ZIP_PATTERNS:
        for zip_path in ROOT.glob(pattern):
            delete_path(zip_path)

    # Remove assignment folders only after extracting any font/DLL fallback.
    delete_path(ROOT / "Assignment9")
    delete_path(ROOT / "Assignment10")

    # Move random loose PNG/JPG previews into docs/previews if they remain and are not game assets.
    previews = ROOT / "docs" / "previews"
    for ext in ("*.png", "*.jpg", "*.jpeg"):
        for p in ROOT.glob(ext):
            if p.name.lower() in {"imagegen.png"}:
                delete_path(p)
            elif p.name not in {"crane_game.exe"}:
                # Root image files are almost certainly old previews, not runtime assets.
                move_file(p, previews / p.name)

    clean_empty_dirs(ROOT)


def main() -> None:
    log("Organizing Crane Operator repo" + (" [dry run]" if DRY_RUN else ""))
    collect_assets()
    patch_crane_game_paths()
    patch_build_script()
    write_repo_docs()
    cleanup_old_files()
    log("\nDone. Next commands:")
    log("  build_crane_game.bat")
    log("  crane_game.exe")
    log("\nThen check:")
    log("  git status")
    log("  git add .")
    log('  git commit -m "Organize project files"')
    log("  git push")


if __name__ == "__main__":
    main()
