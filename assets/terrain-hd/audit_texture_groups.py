#!/usr/bin/env python3
"""Read-only historical game.db audit; write independent scope lists, not HD metadata.

No lab imports, image generation, pixel writes, or changes to shared pack metadata.
Native headers establish availability/storage size only, never release pixel parity.
Python 3.11+ standard library is sufficient.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import hashlib
import json
from pathlib import Path
import re
import struct


def chunks(data):
    offset = 0
    while offset < len(data):
        if offset + 2 > len(data):
            raise ValueError("Truncated structure chunk")
        tag = data[offset]
        width = 4 if data[offset + 1] & 1 else 1
        if offset + 1 + width > len(data):
            raise ValueError("Truncated structure chunk length")
        size = int.from_bytes(data[offset + 1:offset + 1 + width], "little") >> 1
        offset += 1 + width
        if offset + size > len(data):
            raise ValueError("Structure chunk exceeds parent")
        yield tag, data[offset:offset + size]
        offset += size


def read_database(path):
    """Columnar v1 database; resolve serialized handles to registered table IDs."""
    root = dict(chunks(path.read_bytes()))
    if int.from_bytes(root[4], "little") != 1:
        raise ValueError("Expected columnar v1 game.db")
    mapping = list(chunks(dict(chunks(root[1]))[1]))
    handles = {}
    if len(mapping) % 2:
        raise ValueError("Incomplete database table-handle mapping")
    for i in range(0, len(mapping), 2):
        if (mapping[i][0], mapping[i + 1][0]) != (1, 2):
            raise ValueError("Invalid table-handle mapping")
        handles[int.from_bytes(mapping[i + 1][1], "little")] = int.from_bytes(mapping[i][1], "little")
    tables = {}
    for _, entry in chunks(root[2]):
        entry = dict(chunks(entry))
        table_id = handles[int.from_bytes(entry[0], "little")]
        body = dict(chunks(entry[1]))
        columns = {kind: [value.decode("latin1") for _, value in chunks(body[kind])]
                   for kind in (6, 7, 8)}
        groups = []
        for kind in (2, 3, 4):
            rows = []
            for _, packed in chunks(body[kind]):
                if kind == 4:
                    rows.append([value.decode("utf-16-le") for _, value in chunks(packed)])
                else:
                    packed = dict(chunks(packed))
                    count = int.from_bytes(packed[1], "little", signed=True)
                    values = packed.get(2, b"")
                    if count < 0 or len(values) != count * 4:
                        raise ValueError("Invalid packed row")
                    rows.append(list(struct.unpack("<" + ("i" if kind == 2 else "f") * count, values)))
            groups.append(rows)
        count = max(map(len, groups), default=0)
        if any(rows and len(rows) != count for rows in groups):
            raise ValueError("Column group row counts disagree")
        records = []
        for i in range(count):
            record = {}
            for names, rows in zip(columns.values(), groups):
                if rows:
                    if len(names) != len(rows[i]):
                        raise ValueError("Column names and values disagree")
                    record.update(zip(names, rows[i]))
            records.append(record)
        tables[table_id] = {"columns": columns, "records": records}
    return tables


def source_group(texture):
    path = texture.get("SrcName", "").replace("\\", "/").lower()
    prefixes = [("characters/skins/", "characters-clothing"),
                ("characters/equipment/", "equipment"),
                ("characters/weapons/", "weapons"),
                ("heads/", "heads-lshead"), ("lshead/", "heads-lshead"),
                ("interface v5.0/", "interface-v5"), ("effects/", "effects"),
                ("buildings/", "world-remainder"), ("technics/", "world-remainder"),
                ("terrainobjects/", "world-remainder"), ("terrain/", "world-remainder"),
                ("fonts/", "fonts"), ("refmaps/", "reference-maps"),
                ("clues/", "clues"), ("campzones/", "campaign-backgrounds"),
                ("final/", "final"), ("gametutorials/", "tutorials")]
    return next((group for prefix, group in prefixes if path.startswith(prefix)), "other")


def native_header(path):
    if not path.is_file():
        return {"available": False}
    with path.open("rb") as stream:
        data = stream.read(24)
    if len(data) != 24:
        return {"available": True, "error": "truncated MMP header"}
    signature, fmt, average, width, height, mips = struct.unpack("<6I", data)
    if signature != 0x504D4D:
        return {"available": True, "error": "unexpected MMP signature"}
    return {"available": True, "native_format": fmt, "physical_size": [width, height],
            "mips": mips, "file_bytes": path.stat().st_size,
            "release_rgba_match": "not checked by scope audit"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--database", required=True, type=Path)
    parser.add_argument("--textures", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=Path(__file__).parent / "expanded/groups")
    args = parser.parse_args()
    base = Path(__file__).parent
    repo = base.parent.parent
    inventory_path = base / "expanded/full-texture-inventory.json"
    inventory = json.loads(inventory_path.read_text(encoding="utf-8"))
    tables = read_database(args.database)
    db_textures = tables[3]["records"]
    if sorted(db_textures, key=lambda x: x["ID"]) != sorted(inventory, key=lambda x: x["ID"]):
        raise ValueError("Historical DB texture records differ from full inventory")
    textures = {row["ID"]: row for row in db_textures}
    registrations = (repo / "DBFormat/DataFormat.cpp").read_text(encoding="utf-8")
    table_names = {int(number, 0): name for number, name in re.findall(
        r'REGISTER_DATABASE_CLASS\(\s*(0x[0-9a-fA-F]+|[0-9]+),\s*"([^"]+)"', registrations)}
    def named(name):
        matches = [tables[i]["records"] for i, n in table_names.items() if n == name and i in tables]
        if len(matches) != 1:
            raise ValueError(f"Missing/ambiguous table {name}")
        return matches[0]
    materials = named("Materials")
    by_template = defaultdict(list)
    for material in materials:
        by_template[material.get("TemplateID")].append(material)
    geometry = {row["ID"]: row for row in named("Geometries")}
    consumers = defaultdict(list)
    # Model MaterialN, head Material0, race MaterialID and building default material
    # columns point to MaterialTemplates, NOT directly to same-numbered Materials.
    for table_id, table in tables.items():
        name = table_names.get(table_id, f"table-{table_id:#x}")
        for row in table["records"]:
            material_fields = []
            if "GeometryID" in row and "Material0" in row:
                material_fields = [f"Material{i}" for i in range(4)]
            elif "TransformableTextures" in row:
                material_fields = ["Material0"]
            elif "FirstGeometryID" in row:
                material_fields = [key for key in row if "GeomDef" in key and "Material" in key]
            elif "RaceAttributeID" in row:
                material_fields = ["MaterialID"]
            for field in material_fields:
                template = row.get(field)
                if template:
                    item = {"table": name, "id": row["ID"], "field": field,
                            "material_template_id": template}
                    if "GeometryID" in row:
                        item.update(geometry_id=row["GeometryID"],
                                    geometry=geometry.get(row["GeometryID"], {}).get("SrcName", ""),
                                    model_template_id=row.get("TemplateID"), flags=row.get("Flags", ""))
                    elif "TransformableTextures" in row:
                        item.update(head_source=row.get("SrcName", ""),
                                    transformable=bool(row.get("IsTransformable")),
                                    transformable_textures_id=row.get("TransformableTextures"))
                    for material in by_template[template]:
                        consumers[material["ID"]].append(item)
    refs = defaultdict(list)
    particle_definitions = {row["ID"]: row for row in named("Particles")}
    technical_fields = {"BumpID", "GlossID", "MirrorID", "MaskID", "SLightMaskID"}
    direct_tables = {"Materials", "Fonts", "TerrainTiles", "Particles", "ParticleInstances",
                     "UITextures", "CubeTextures", "TransformableFaceTextures", "TransformableEyeTextures",
                     "TransformableEyelashTeethTextures", "Spots", "Grass", "Objects", "PlacableObjects"}
    for table_id, table in tables.items():
        name = table_names.get(table_id, f"table-{table_id:#x}")
        for row in table["records"]:
            fields = []
            if name == "Materials":
                fields = ["TextureID", "BumpID", "GlossID", "MirrorID"]
            elif name == "UITextures":
                fields = [key for key in row if key.startswith("R_")]
            elif name == "ParticleInstances":
                fields = [key for key in row if re.fullmatch(r"(?:Bump)?Texture\d+", key)]
            elif name in direct_tables:
                fields = [key for key in row if key in {"TextureID", "BumpID", "MaskID", "LightFlareTexture",
                          "PLightFlareTexture", "SLightMaskID"} or re.fullmatch(r"Texture\d+", key)]
            # These lighting fields are direct CTexture links regardless of table name.
            fields += [key for key in ("LightFlareTexture", "PLightFlareTexture", "SLightMaskID")
                       if key in row and key not in fields]
            for field in fields:
                texture_id = row.get(field)
                if texture_id not in textures:
                    continue
                technical = field in technical_fields or field.startswith("BumpTexture")
                item = {"table": name, "id": row["ID"], "field": field,
                        "role": "technical" if technical else "color"}
                for key in ("UserName", "SrcName", "Alpha", "AddressMode", "TemplateID", "THMID",
                            "Priority", "TextureName", "EffectID", "ParticleID", "IsCrown",
                            "AlphaBlending", "Static", "PivotX", "PivotY", "Scale", "Speed", "CycleCount"):
                    if key in row:
                        item[key] = row[key]
                for key, value in row.items():
                    if any(word in key.lower() for word in ("frame", "column", "row", "tile", "sidesize", "wrap")):
                        item[key] = value
                if name == "ParticleInstances":
                    item["particle_definition"] = particle_definitions.get(row.get("ParticleID"), {})
                    item["particle_reference_status"] = "resolved" if item["particle_definition"] else "missing-definition-requires-usage-review"
                    item["frame_bindings"] = {key: value for key, value in row.items()
                                              if re.fullmatch(r"Texture\d+", key) and value > 0}
                    item["layout_review"] = "Texture slots are animation bindings, not inferred atlas cells. Preserve Particle.WrapX/WrapY, instance pivot, timing, blending and any native particle UV grid; inspect source before defining calibration_grid."
                if name == "Materials":
                    item["consumers"] = consumers[row["ID"]]
                refs[texture_id].append(item)
    ui_rows = named("UITextures")
    cursors = named("UICursors")
    controls = named("UIControls")
    ui_by_id = {row["ID"]: row for row in ui_rows}
    ui_uses = defaultdict(list)
    for row in controls:
        for field in [f"Texture{i}" for i in range(6)]:
            if row.get(field) in ui_by_id:
                ui_uses[row[field]].append({"table": "UIControls", "id": row["ID"], "field": field,
                    "control_name": row.get("IDText", ""), "type": row["Type"],
                    "rect": [row[k] for k in ("Left", "Top", "Right", "Bottom")],
                    "container": row.get("UIContainerID"), "string_id": row.get("StringID")})
    # Other explicit UITexture-typed columns. Do not confuse IDs across tables.
    for name, fields in {"Nationalities": ["IconNormalTexture", "IconDisabledTexture", "FlagTexture",
                                            "CharGenBackgroundID", "CustomHeadBackgroundID"],
                         "UICursors": ["UITexture"], "Medals": ["ImageID"],
                         "RPGItems": ["Icon", "IconDisabled"], "RPGPerks": ["Icon", "IconDisabled"],
                         "ChapterMaps": ["Background", "PWLImageID"],
                         "ScenarioZones": ["PWLImageID"]}.items():
        ids = [i for i, n in table_names.items() if n == name and i in tables]
        for i in ids:
            for row in tables[i]["records"]:
                for field in fields:
                    if row.get(field) in ui_by_id:
                        ui_uses[row[field]].append({"table": name, "id": row["ID"], "field": field})
    ui_links = defaultdict(list)
    cursor_ids = set()
    for row in ui_rows:
        selected = next((field for field in ["R_1024x768", "R_1600x1200", "R_1280x960", "R_800x600"]
                         if row.get(field) in textures), None)
        logical = [0, 0]
        if selected:
            t = textures[row[selected]]
            divisor = {"R_1024x768": (1024, 768), "R_1600x1200": (1600, 1200),
                       "R_1280x960": (1280, 1024), "R_800x600": (800, 600)}[selected]
            logical = [t["Width"] * 1024 // divisor[0], t["Height"] * 768 // divisor[1]]
        linked_cursors = [{key: value for key, value in cursor.items()} for cursor in cursors
                          if cursor.get("UITexture") == row["ID"]]
        for field in ["R_800x600", "R_1024x768", "R_1280x960", "R_1600x1200"]:
            texture_id = row.get(field)
            if texture_id in textures:
                ui_links[texture_id].append({"ui_texture_id": row["ID"], "name": row.get("UserName", ""),
                    "resolution_field": field, "ui_logical_size": logical,
                    "logical_size_from": selected, "controls_and_uses": ui_uses[row["ID"]],
                    "cursors": linked_cursors})
                if linked_cursors:
                    cursor_ids.add(texture_id)
    queue = json.loads((base / "expanded/queue.json").read_text(encoding="utf-8"))
    queue = {row["id"]: row for row in queue}
    accepted = json.loads((base / "sources.json").read_text(encoding="utf-8"))["textures"]
    accepted_ids = {row["id"] for row in accepted}
    for row in accepted:
        accepted_ids.update(row.get("aliases", []))
    groups = defaultdict(list)
    for texture_id, texture in sorted(textures.items()):
        related = refs[texture_id]
        roles = {ref["role"] for ref in related}
        header = native_header(args.textures / str(texture_id))
        reasons = []
        if texture.get("Type", "").lower() == "bump" or texture.get("Format", "").lower() in {"normal", "normals"}:
            reasons.append("explicit-bump-normal")
        if roles == {"technical"}:
            reasons.append("only-technical-direct-uses")
        if texture_id in accepted_ids:
            disposition = "accepted-do-not-regenerate"
        elif texture_id in queue:
            disposition = "existing-queue-disposition"
        elif reasons:
            disposition = "preserve-technical"
        elif roles == {"color", "technical"}:
            disposition = "mixed-color-control-audit-required"
        elif source_group(texture) == "fonts":
            disposition = "font-rendering-audit-no-imagegen"
        elif not header["available"] or header.get("error"):
            disposition = "source-unavailable-or-invalid"
        elif texture_id in cursor_ids or "/cursors/" in texture["SrcName"].replace("\\", "/").lower():
            disposition = "cursor-runtime-audit-required"
        elif not related:
            disposition = "usage-audit-required"
        else:
            disposition = "candidate-source-pixel-validation-required"
        record = {"id": texture_id, "texture": texture, "logical_size": [texture["Width"], texture["Height"]],
                  "historical_header": header, "disposition": disposition,
                  "technical_reasons": reasons, "direct_texture_uses": related,
                  "ui_links": ui_links[texture_id],
                  "source_pixel_validation": "required before adding to generation queue",
                  "layout_validation": "private original/UV/atlas inspection required; no guessed layout"}
        if texture_id in queue:
            record["shared_queue_status"] = queue[texture_id]["status"]
        if "technical" in roles and "color" in roles:
            record["mixed_role_warning"] = "Shared color/control resource: keep native original unless separately resolved"
        groups[source_group(texture)].append(record)
    args.output.mkdir(parents=True, exist_ok=True)
    candidate_directory = args.output / "candidates"
    candidate_directory.mkdir(exist_ok=True)
    summary = {"version": 1, "scope": "read-only candidate audit; not a generation queue or all-game completion",
               "database_sha256": hashlib.sha256(args.database.read_bytes()).hexdigest(),
               "inventory_sha256": hashlib.sha256(inventory_path.read_bytes()).hexdigest(),
               "inventory_exactly_matches_database": True, "total_texture_records": len(textures),
               "table_count": len(tables), "command": "python assets/terrain-hd/audit_texture_groups.py --database <historical-game.db> --textures <historical-Textures> --output assets/terrain-hd/expanded/groups",
               "limitations": ["Paths select groups, while typed DB links establish uses; lack of a direct link does not prove unused.",
                                "Headers are not decoded pixels. Release RGBA parity and structural/solid-mask inspection remain mandatory.",
                                "Model/head material slots resolve through MaterialTemplates -> Materials.TemplateID.",
                                "BRDFID points to BRDF records, not a texture; UIControls.TextureN points to UITextures, not a texture.",
                                "External script/native-resource references are not exhaustively interpreted.",
                                "UI logical dimensions are computed from original DB texture sizes; preserve DB sizes/coordinates.",
                                "Hardware cursor reader currently samples an original-size corner of physical HD storage; defer cursor assets pending runtime fix/check.",
                                "Raster/font atlas references require font-rendering audit; vector fonts are excluded from imagegen.",
                                "Native RGB565/CF_R5G6B5 is color; Format=565 alone is never an exclusion."],
               "groups": {}}
    for group, records in sorted(groups.items()):
        filename = group + ".json"
        (args.output / filename).write_text(json.dumps(records, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        candidates = [{"texture": r["texture"], "category": group, "role": "typed-db-color-candidate",
                       "usage": {"direct_texture_uses": r["direct_texture_uses"], "ui_links": r["ui_links"]},
                       "review_requirements": ["Inspect decoded original before imagegen; resolve UV and atlas layout.",
                                               "Preserve original logical size, alpha, material, repeat/detail scale and character identity.",
                                               "Do not infer acceptance from historical/release equality; inspect normalized generation privately."]}
                      for r in records if r["disposition"] in {"candidate-source-pixel-validation-required",
                                                              "source-unavailable-or-invalid"}]
        (candidate_directory / filename).write_text(json.dumps(candidates, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        summary["groups"][group] = {"file": filename, "total": len(records),
            "candidate_input": "candidates/" + filename, "source_validation_candidates": len(candidates),
            "without_explicit_bump": sum(not (r["texture"]["Type"].lower() == "bump" or r["texture"]["Format"].lower() in {"normal", "normals"}) for r in records),
            "dispositions": dict(sorted(Counter(r["disposition"] for r in records).items())),
            "color_rgb565_ids": [r["id"] for r in records if r["texture"]["Format"] == "565" and r["texture"]["Type"].lower() != "bump"],
            "mixed_role_ids": [r["id"] for r in records if "mixed_role_warning" in r]}
    (args.output / "audit-summary.json").write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
