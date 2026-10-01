from pathlib import Path
from PIL import Image, ImageDraw

#各クリップの4時点を並べ、全身の姿勢と固定あり・なしの差を見比べる。
folder = Path(__file__).resolve().parents[1] / "Saved/Tests/EnemyClips"
for enemy in ("BP_Enemy", "BP_MidBossChara", "BP_FinalBossChara"):
    names = sorted({p.name.rsplit("_lock", 1)[0] for p in folder.glob(enemy + "_*_lock0_0.png")})
    for start in range(0, len(names), 4):
        group = names[start:start + 4]
        sheet = Image.new("RGB", (1536, len(group) * 210), "#242424")
        draw = ImageDraw.Draw(sheet)
        for row, name in enumerate(group):
            draw.text((8, row * 210 + 3), name + " | source / old lock / corrected runtime", fill="white")
            for lock in range(3):
                for frame in range(4):
                    file = folder / f"{name}_lock{lock}_{frame}.png"
                    if not file.exists():
                        continue
                    shot = Image.open(file).convert("RGB").resize((128, 180))
                    sheet.paste(shot, ((lock * 4 + frame) * 128, row * 210 + 26))
        sheet.save(folder / f"{enemy}_sheet{start // 4}.jpg")

#攻撃姿勢の差を大きく表示し、腰の固定による脚と腕のずれを確認しやすくする。
examples = [("BP_Enemy_Zombie_Punching", "MINION"),
            ("BP_MidBossChara_Mutant_Punch", "MID BOSS"),
            ("BP_FinalBossChara_Mutant_Punch", "FINAL BOSS")]
comparison = Image.new("RGB", (768, 1242), "#242424")
labels = ImageDraw.Draw(comparison)
for row, (name, label) in enumerate(examples):
    labels.text((12, row * 414 + 8), label + " / BEFORE: pelvis locked", fill="white")
    labels.text((396, row * 414 + 8), "AFTER: corrected runtime pose", fill="white")
    for column, lock in enumerate((1, 2)):
        file = folder / f"{name}_lock{lock}_1.png"
        if file.exists():
            comparison.paste(Image.open(file).convert("RGB"), (column * 384, row * 414 + 30))
comparison.save(folder / "AttackBeforeAfter.jpg")
