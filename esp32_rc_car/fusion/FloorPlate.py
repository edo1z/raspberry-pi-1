# Fusion 360 Python Script - RC Car Floor Plate
# 5mm厚、電池ボックス下にザグリ付き

import adsk.core, adsk.fusion, traceback

def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        # 単位: cm (Fusion 360のデフォルト)
        def mm(val):
            return val / 10.0

        # パラメータ
        floor_length = mm(180)
        floor_width = mm(80)
        floor_thickness = mm(5)  # 5mmに変更

        # ESP32 寸法（縦向き配置: 長さ57mmがY方向）
        esp32_length = mm(57)   # Y方向（縦）
        esp32_width = mm(28)    # X方向（横）
        esp32_wall_thickness = mm(2)
        esp32_wall_height = mm(7)

        # 電池ボックス 寸法
        battery_size = mm(68)
        battery_wall_thickness = mm(2)
        battery_wall_height = mm(12)

        # モータードライバ 寸法
        motor_driver_hole_distance = mm(41)
        m3_hole_diameter = mm(3.5)

        # 配線用長穴（X方向に短く、Y方向に長い）
        cable_hole_length = mm(5)   # X方向（細く）
        cable_hole_width = mm(60)   # Y方向

        # スペーサー用長穴（中央に配置）
        spacer_slot_width = mm(3.5)   # M3用
        spacer_slot_length = mm(60)   # Y方向
        spacer_slot_spacing = mm(44)  # X方向の間隔

        # ザグリ（ネジ頭用の凹み）
        counterbore_diameter = mm(7)  # M3ネジ頭用（6mm + 余裕）
        counterbore_depth = mm(3)     # ネジ頭の高さ分

        # 配置位置（ESP32は縦向き: widthがX方向）
        esp32_x = -floor_length/2 + mm(5) + esp32_width/2
        motor_driver_x = esp32_x + esp32_width/2 + mm(20) + mm(43)/2
        battery_x = floor_length/2 - mm(5) - battery_size/2
        cable_hole_x = (esp32_x + esp32_width/2 + motor_driver_x - mm(43)/2) / 2

        sketches = rootComp.sketches
        xyPlane = rootComp.xYConstructionPlane
        extrudes = rootComp.features.extrudeFeatures

        # ============================================
        # 1. 床板ベース（5mm厚）
        # ============================================
        sketch1 = sketches.add(xyPlane)
        sketch1.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(-floor_length/2, -floor_width/2, 0),
            adsk.core.Point3D.create(floor_length/2, floor_width/2, 0)
        )
        prof1 = sketch1.profiles.item(0)
        extInput1 = extrudes.createInput(prof1, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
        extInput1.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness))
        extrudes.add(extInput1)

        # ============================================
        # 2. ESP32ホルダー壁（縦向き: 前後の壁）
        # ============================================
        for side in [1, -1]:
            sketch_wall = sketches.add(xyPlane)
            wall_y = side * (esp32_length/2 + esp32_wall_thickness/2)
            sketch_wall.sketchCurves.sketchLines.addTwoPointRectangle(
                adsk.core.Point3D.create(esp32_x - esp32_width/2, wall_y - esp32_wall_thickness/2, 0),
                adsk.core.Point3D.create(esp32_x + esp32_width/2, wall_y + esp32_wall_thickness/2, 0)
            )
            prof_wall = sketch_wall.profiles.item(0)
            extInput_wall = extrudes.createInput(prof_wall, adsk.fusion.FeatureOperations.JoinFeatureOperation)
            extInput_wall.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + esp32_wall_height))
            extrudes.add(extInput_wall)

        # ============================================
        # 3. 電池ボックスホルダー（4面、左壁に30mm隙間）
        # ============================================
        battery_gap = mm(30)  # 左壁中央の隙間

        # 前壁（+Y側）- 内側のみ
        sketch_bf = sketches.add(xyPlane)
        sketch_bf.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(battery_x - battery_size/2, battery_size/2, 0),
            adsk.core.Point3D.create(battery_x + battery_size/2, battery_size/2 + battery_wall_thickness, 0)
        )
        prof_bf = sketch_bf.profiles.item(0)
        ext_bf = extrudes.createInput(prof_bf, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        ext_bf.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + battery_wall_height))
        extrudes.add(ext_bf)

        # 後壁（-Y側）- 内側のみ
        sketch_bb = sketches.add(xyPlane)
        sketch_bb.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(battery_x - battery_size/2, -battery_size/2 - battery_wall_thickness, 0),
            adsk.core.Point3D.create(battery_x + battery_size/2, -battery_size/2, 0)
        )
        prof_bb = sketch_bb.profiles.item(0)
        ext_bb = extrudes.createInput(prof_bb, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        ext_bb.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + battery_wall_height))
        extrudes.add(ext_bb)

        # 右壁（+X側）- 前壁・後壁とつながるように伸ばす
        sketch_br = sketches.add(xyPlane)
        sketch_br.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(battery_x + battery_size/2, -battery_size/2 - battery_wall_thickness, 0),
            adsk.core.Point3D.create(battery_x + battery_size/2 + battery_wall_thickness, battery_size/2 + battery_wall_thickness, 0)
        )
        prof_br = sketch_br.profiles.item(0)
        ext_br = extrudes.createInput(prof_br, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        ext_br.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + battery_wall_height))
        extrudes.add(ext_br)

        # 左壁・上部（+Y側、隙間より上）- 前壁とつながるように伸ばす
        sketch_bl_t = sketches.add(xyPlane)
        bl_x1 = battery_x - battery_size/2 - battery_wall_thickness
        bl_x2 = battery_x - battery_size/2
        sketch_bl_t.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(bl_x1, battery_gap/2, 0),
            adsk.core.Point3D.create(bl_x2, battery_size/2 + battery_wall_thickness, 0)  # 前壁の外側まで
        )
        prof_bl_t = sketch_bl_t.profiles.item(0)
        ext_bl_t = extrudes.createInput(prof_bl_t, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        ext_bl_t.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + battery_wall_height))
        extrudes.add(ext_bl_t)

        # 左壁・下部（-Y側、隙間より下）- 後壁とつながるように伸ばす
        sketch_bl_b = sketches.add(xyPlane)
        sketch_bl_b.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(bl_x1, -battery_size/2 - battery_wall_thickness, 0),  # 後壁の外側まで
            adsk.core.Point3D.create(bl_x2, -battery_gap/2, 0)
        )
        prof_bl_b = sketch_bl_b.profiles.item(0)
        ext_bl_b = extrudes.createInput(prof_bl_b, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        ext_bl_b.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + battery_wall_height))
        extrudes.add(ext_bl_b)

        # ============================================
        # 4. モータードライバM3穴
        # ============================================
        for dx in [-1, 1]:
            for dy in [-1, 1]:
                sketch_hole = sketches.add(xyPlane)
                sketch_hole.sketchCurves.sketchCircles.addByCenterRadius(
                    adsk.core.Point3D.create(motor_driver_x + dx*motor_driver_hole_distance/2, dy*motor_driver_hole_distance/2, 0),
                    m3_hole_diameter / 2
                )
                prof_hole = sketch_hole.profiles.item(0)
                ext_hole = extrudes.createInput(prof_hole, adsk.fusion.FeatureOperations.CutFeatureOperation)
                ext_hole.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + mm(1)))
                extrudes.add(ext_hole)

        # ============================================
        # 5. 配線用長穴（ESP32とモータードライバの間）
        # ============================================
        sketch_cable = sketches.add(xyPlane)
        sketch_cable.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(cable_hole_x - cable_hole_length/2, -cable_hole_width/2, 0),
            adsk.core.Point3D.create(cable_hole_x + cable_hole_length/2, cable_hole_width/2, 0)
        )
        prof_c = sketch_cable.profiles.item(0)
        ext_c = extrudes.createInput(prof_c, adsk.fusion.FeatureOperations.CutFeatureOperation)
        ext_c.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + mm(1)))
        extrudes.add(ext_c)

        # ============================================
        # 6. スペーサー用長穴（中央に2本）
        # ============================================
        for dx in [-1, 1]:
            sketch_spacer = sketches.add(xyPlane)
            slot_x = dx * spacer_slot_spacing / 2
            sketch_spacer.sketchCurves.sketchLines.addTwoPointRectangle(
                adsk.core.Point3D.create(slot_x - spacer_slot_width/2, -spacer_slot_length/2, 0),
                adsk.core.Point3D.create(slot_x + spacer_slot_width/2, spacer_slot_length/2, 0)
            )
            prof_spacer = sketch_spacer.profiles.item(0)
            ext_spacer = extrudes.createInput(prof_spacer, adsk.fusion.FeatureOperations.CutFeatureOperation)
            ext_spacer.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + mm(1)))
            extrudes.add(ext_spacer)

        # ============================================
        # 7. 電池ボックス下のザグリ（ネジ頭用の凹み）
        # ============================================
        # 電池ボックスエリア内でスペーサー長穴と重なる位置
        # 右側の長穴（+22mm）が電池ボックスエリア（左端約+17mm）にかぶる
        # XY平面から上に向かって（floor_thickness - counterbore_depth）だけ押し出して
        # 残りの部分を凹みとして残す方式ではなく、
        # 長穴の周りに段差をつける（長穴を広げる形でザグリ）

        # 右側のスペーサー長穴位置（電池ボックスとかぶる）
        right_slot_x = spacer_slot_spacing / 2  # +22mm

        # ザグリ用の広い長穴を上面から counterbore_depth 分だけカット
        # まず上面に構築平面を作成
        planes = rootComp.constructionPlanes
        planeInput = planes.createInput()
        offsetValue = adsk.core.ValueInput.createByReal(floor_thickness - counterbore_depth)
        planeInput.setByOffset(xyPlane, offsetValue)
        zaguri_plane = planes.add(planeInput)

        sketch_zag = sketches.add(zaguri_plane)
        zaguri_margin = mm(2)
        sketch_zag.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(right_slot_x - counterbore_diameter/2, -spacer_slot_length/2 - zaguri_margin, 0),
            adsk.core.Point3D.create(right_slot_x + counterbore_diameter/2, spacer_slot_length/2 + zaguri_margin, 0)
        )
        prof_zag = sketch_zag.profiles.item(0)
        ext_zag = extrudes.createInput(prof_zag, adsk.fusion.FeatureOperations.CutFeatureOperation)
        ext_zag.setDistanceExtent(False, adsk.core.ValueInput.createByReal(counterbore_depth + mm(1)))
        extrudes.add(ext_zag)

        # ============================================
        # 8. ESP32のピン用穴（左右2列）
        # ============================================
        # ESP32は縦向き（Y方向に57mm）、ピンは左右の端に沿って並ぶ
        pin_hole_length = esp32_length - mm(4)  # Y方向の長さ（53mm）
        pin_hole_width = mm(3)  # X方向の幅

        # 左側のピン穴（-X側）
        sketch_pin_l = sketches.add(xyPlane)
        pin_x_left = esp32_x - esp32_width/2 + mm(1.5)
        sketch_pin_l.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(pin_x_left - pin_hole_width/2, -pin_hole_length/2, 0),
            adsk.core.Point3D.create(pin_x_left + pin_hole_width/2, pin_hole_length/2, 0)
        )
        prof_pin_l = sketch_pin_l.profiles.item(0)
        ext_pin_l = extrudes.createInput(prof_pin_l, adsk.fusion.FeatureOperations.CutFeatureOperation)
        ext_pin_l.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + mm(1)))
        extrudes.add(ext_pin_l)

        # 右側のピン穴（+X側）
        sketch_pin_r = sketches.add(xyPlane)
        pin_x_right = esp32_x + esp32_width/2 - mm(1.5)
        sketch_pin_r.sketchCurves.sketchLines.addTwoPointRectangle(
            adsk.core.Point3D.create(pin_x_right - pin_hole_width/2, -pin_hole_length/2, 0),
            adsk.core.Point3D.create(pin_x_right + pin_hole_width/2, pin_hole_length/2, 0)
        )
        prof_pin_r = sketch_pin_r.profiles.item(0)
        ext_pin_r = extrudes.createInput(prof_pin_r, adsk.fusion.FeatureOperations.CutFeatureOperation)
        ext_pin_r.setDistanceExtent(False, adsk.core.ValueInput.createByReal(floor_thickness + mm(1)))
        extrudes.add(ext_pin_r)

        ui.messageBox('Floor Plate created!\n\n' +
                     '- Floor: 180x80x5mm\n' +
                     '- ESP32 holder (vertical)\n' +
                     '- Battery box holder\n' +
                     '- Motor driver M3 holes\n' +
                     '- Cable routing hole\n' +
                     '- Spacer slots (44mm apart, 60mm long)\n' +
                     '- Counterbore for screw heads under battery\n\n' +
                     '爪は後でFusion 360のGUIで追加してください')

    except:
        if ui:
            ui.messageBox('Error:\n{}'.format(traceback.format_exc()))
