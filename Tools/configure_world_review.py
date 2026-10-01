import os
import runpy

#素材、後退の再生設定、マップ照明の順に保存し、依存する素材を先に用意する。
for script in ('create_goal_material.py', 'configure_backward_assets.py', 'tune_forest_lighting.py'):
    runpy.run_path(os.path.join(os.path.dirname(__file__), script))
