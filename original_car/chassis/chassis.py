# Fusion 360 スクリプト - シャーシ（Chassis）
# 3輪車のベースプレート

import adsk.core, adsk.fusion, adsk.cam, traceback, math

def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        # === パラメータ設定（mm） ===
        chassis_width = 100      # シャーシ幅
        chassis_length = 150     # シャーシ長さ
        chassis_thickness = 5    # シャーシ厚み

        # 前輪軸受け取付位置
        front_axle_offset = 20   # 前端からの距離

        # 後輪モーター取付位置
        rear_axle_offset = 20    # 後端からの距離
        motor_mount_width = 25   # モーターマウント幅
        motor_mount_length = 35  # モーターマウント長さ

        # 軸受け取付穴
        axle_holder_hole_spacing = 14  # 軸受け取付穴の間隔
        axle_holder_hole_diameter = 3  # 取付穴直径

        # 電池・基板スペース
        battery_cutout_width = 60    # 電池スペース幅
        battery_cutout_length = 40   # 電池スペース長さ

        # 単位をcmに変換
        c_width = chassis_width / 10
        c_length = chassis_length / 10
        c_thick = chassis_thickness / 10
        front_offset = front_axle_offset / 10
        rear_offset = rear_axle_offset / 10
        m_width = motor_mount_width / 10
        m_length = motor_mount_length / 10
        ah_spacing = axle_holder_hole_spacing / 10
        ah_hole_r = (axle_holder_hole_diameter / 2) / 10
        bat_width = battery_cutout_width / 10
        bat_length = battery_cutout_length / 10

        # ========== コンポーネントを作成 ==========
        occ = rootComp.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        chassisComp = occ.component
        chassisComp.name = "Chassis"

        sketches = chassisComp.sketches
        xyPlane = chassisComp.xYConstructionPlane
        extrudes = chassisComp.features.extrudeFeatures

        # ========== ベースプレートを作成 ==========
        sketch1 = sketches.add(xyPlane)
        lines = sketch1.sketchCurves.sketchLines

        # シャーシの四角形（中心を原点に）
        half_w = c_width / 2
        half_l = c_length / 2

        p1 = adsk.core.Point3D.create(-half_w, -half_l, 0)
        p2 = adsk.core.Point3D.create(half_w, -half_l, 0)
        p3 = adsk.core.Point3D.create(half_w, half_l, 0)
        p4 = adsk.core.Point3D.create(-half_w, half_l, 0)

        lines.addByTwoPoints(p1, p2)
        lines.addByTwoPoints(p2, p3)
        lines.addByTwoPoints(p3, p4)
        lines.addByTwoPoints(p4, p1)

        baseProfile = sketch1.profiles.item(0)
        extInput = extrudes.createInput(baseProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
        extInput.setDistanceExtent(False, adsk.core.ValueInput.createByReal(c_thick))
        extrudes.add(extInput)

        # ========== 前輪軸受け取付穴（左側）==========
        sketch2 = sketches.add(xyPlane)
        front_y = half_l - front_offset

        # 左前の取付穴（2つ）
        hole1 = adsk.core.Point3D.create(-half_w + 0.03 + ah_hole_r, front_y - ah_spacing/2, 0)
        hole2 = adsk.core.Point3D.create(-half_w + 0.03 + ah_hole_r, front_y + ah_spacing/2, 0)
        sketch2.sketchCurves.sketchCircles.addByCenterRadius(hole1, ah_hole_r)
        sketch2.sketchCurves.sketchCircles.addByCenterRadius(hole2, ah_hole_r)

        for i in range(sketch2.profiles.count):
            holeProfile = sketch2.profiles.item(i)
            extHole = extrudes.createInput(holeProfile, adsk.fusion.FeatureOperations.CutFeatureOperation)
            extHole.setDistanceExtent(False, adsk.core.ValueInput.createByReal(c_thick + 0.1))
            extrudes.add(extHole)

        # ========== 前輪軸受け取付穴（右側）==========
        sketch3 = sketches.add(xyPlane)

        # 右前の取付穴（2つ）
        hole3 = adsk.core.Point3D.create(half_w - 0.03 - ah_hole_r, front_y - ah_spacing/2, 0)
        hole4 = adsk.core.Point3D.create(half_w - 0.03 - ah_hole_r, front_y + ah_spacing/2, 0)
        sketch3.sketchCurves.sketchCircles.addByCenterRadius(hole3, ah_hole_r)
        sketch3.sketchCurves.sketchCircles.addByCenterRadius(hole4, ah_hole_r)

        for i in range(sketch3.profiles.count):
            holeProfile = sketch3.profiles.item(i)
            extHole = extrudes.createInput(holeProfile, adsk.fusion.FeatureOperations.CutFeatureOperation)
            extHole.setDistanceExtent(False, adsk.core.ValueInput.createByReal(c_thick + 0.1))
            extrudes.add(extHole)

        # ========== 後輪モーターマウント穴（左側）==========
        sketch4 = sketches.add(xyPlane)
        rear_y = -half_l + rear_offset

        # 左後のモーター取付穴（4つの角）
        m_half_w = m_width / 2
        m_half_l = m_length / 2
        motor_x = -half_w + m_half_w + 0.05

        mh1 = adsk.core.Point3D.create(motor_x - m_half_w + 0.03, rear_y - m_half_l + 0.03, 0)
        mh2 = adsk.core.Point3D.create(motor_x + m_half_w - 0.03, rear_y - m_half_l + 0.03, 0)
        mh3 = adsk.core.Point3D.create(motor_x - m_half_w + 0.03, rear_y + m_half_l - 0.03, 0)
        mh4 = adsk.core.Point3D.create(motor_x + m_half_w - 0.03, rear_y + m_half_l - 0.03, 0)

        sketch4.sketchCurves.sketchCircles.addByCenterRadius(mh1, ah_hole_r)
        sketch4.sketchCurves.sketchCircles.addByCenterRadius(mh2, ah_hole_r)
        sketch4.sketchCurves.sketchCircles.addByCenterRadius(mh3, ah_hole_r)
        sketch4.sketchCurves.sketchCircles.addByCenterRadius(mh4, ah_hole_r)

        for i in range(sketch4.profiles.count):
            holeProfile = sketch4.profiles.item(i)
            extHole = extrudes.createInput(holeProfile, adsk.fusion.FeatureOperations.CutFeatureOperation)
            extHole.setDistanceExtent(False, adsk.core.ValueInput.createByReal(c_thick + 0.1))
            extrudes.add(extHole)

        # ========== 後輪モーターマウント穴（右側）==========
        sketch5 = sketches.add(xyPlane)
        motor_x_r = half_w - m_half_w - 0.05

        mh5 = adsk.core.Point3D.create(motor_x_r - m_half_w + 0.03, rear_y - m_half_l + 0.03, 0)
        mh6 = adsk.core.Point3D.create(motor_x_r + m_half_w - 0.03, rear_y - m_half_l + 0.03, 0)
        mh7 = adsk.core.Point3D.create(motor_x_r - m_half_w + 0.03, rear_y + m_half_l - 0.03, 0)
        mh8 = adsk.core.Point3D.create(motor_x_r + m_half_w - 0.03, rear_y + m_half_l - 0.03, 0)

        sketch5.sketchCurves.sketchCircles.addByCenterRadius(mh5, ah_hole_r)
        sketch5.sketchCurves.sketchCircles.addByCenterRadius(mh6, ah_hole_r)
        sketch5.sketchCurves.sketchCircles.addByCenterRadius(mh7, ah_hole_r)
        sketch5.sketchCurves.sketchCircles.addByCenterRadius(mh8, ah_hole_r)

        for i in range(sketch5.profiles.count):
            holeProfile = sketch5.profiles.item(i)
            extHole = extrudes.createInput(holeProfile, adsk.fusion.FeatureOperations.CutFeatureOperation)
            extHole.setDistanceExtent(False, adsk.core.ValueInput.createByReal(c_thick + 0.1))
            extrudes.add(extHole)

        # ========== 中央の軽量化穴（電池スペース兼用）==========
        sketch6 = sketches.add(xyPlane)
        rect_lines = sketch6.sketchCurves.sketchLines

        bat_half_w = bat_width / 2
        bat_half_l = bat_length / 2

        bp1 = adsk.core.Point3D.create(-bat_half_w, -bat_half_l, 0)
        bp2 = adsk.core.Point3D.create(bat_half_w, -bat_half_l, 0)
        bp3 = adsk.core.Point3D.create(bat_half_w, bat_half_l, 0)
        bp4 = adsk.core.Point3D.create(-bat_half_w, bat_half_l, 0)

        rect_lines.addByTwoPoints(bp1, bp2)
        rect_lines.addByTwoPoints(bp2, bp3)
        rect_lines.addByTwoPoints(bp3, bp4)
        rect_lines.addByTwoPoints(bp4, bp1)

        cutoutProfile = sketch6.profiles.item(0)
        extCutout = extrudes.createInput(cutoutProfile, adsk.fusion.FeatureOperations.CutFeatureOperation)
        extCutout.setDistanceExtent(False, adsk.core.ValueInput.createByReal(c_thick + 0.1))
        extrudes.add(extCutout)

        # ビューをフィット
        viewport = app.activeViewport
        viewport.fit()

        ui.messageBox(f'シャーシ（Chassis）完成！\n\n幅: {chassis_width}mm\n長さ: {chassis_length}mm\n厚み: {chassis_thickness}mm\n\n前輪軸受け取付穴: 左右各2個\n後輪モーター取付穴: 左右各4個\n中央に軽量化穴（電池スペース）\n\nコンポーネント名: Chassis')

    except:
        if ui:
            ui.messageBox('エラーが発生しました:\n{}'.format(traceback.format_exc()))
