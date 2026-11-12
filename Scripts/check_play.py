import unreal
import json
from pathlib import Path

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
pawn = unreal.GameplayStatics.get_player_character(world, 0)
controller = unreal.GameplayStatics.get_player_controller(world, 0)
def vec(v): return [v.x, v.y, v.z]
report = {'world': world.get_path_name(), 'pawn': pawn.get_class().get_path_name(), 'location': vec(pawn.get_actor_location()), 'velocity': vec(pawn.get_velocity()), 'mode': str(pawn.character_movement.movement_mode), 'controller': str(controller), 'components': []}
for c in pawn.get_components_by_class(unreal.ActorComponent):
    report['components'].append({'name': c.get_name(), 'class': c.get_class().get_name()})
report['input_axes'] = [str(x) for x in unreal.InputSettings.get_input_settings().get_axis_mappings()]
report['input_actions'] = [str(x) for x in unreal.InputSettings.get_input_settings().get_action_mappings()]
Path(unreal.Paths.project_dir(), 'Saved', 'play_report.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
unreal.log('PLAY_REPORT_SAVED')
