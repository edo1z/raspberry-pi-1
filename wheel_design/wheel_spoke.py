# Fusion 360 スクリプト - スポーク型ホイール（コンポーネント版）
# タイヤのゴムを再利用するためのホイール

import adsk.core, adsk.fusion, adsk.cam, traceback, math

def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        # === パラメータ設定（mm） ===
        outer_diameter = 51      # 外径 - ゴムがはまる
        ring_width = 5           # 外周リングの幅
        hub_diameter = 15        # 中心ハブ外径
        axle_hole = 5            # 軸穴
        spoke_width = 6          # スポーク幅
        wheel_thickness = 25     # 全体の厚み
        num_spokes = 3           # スポーク本数
        spoke_overlap = 3        # スポークがハブに食い込む量（mm）

        # 単位をcmに変換（Fusion 360の内部単位）
        outer_r = (outer_diameter / 2) / 10
        inner_ring_r = (outer_diameter / 2 - ring_width) / 10
        hub_r = (hub_diameter / 2) / 10
        axle_r = (axle_hole / 2) / 10
        spoke_w = spoke_width / 10
        thickness = wheel_thickness / 10
        overlap = spoke_overlap / 10

        # スポークの開始位置（ハブの中心寄りに食い込む）
        spoke_inner_r = hub_r - overlap
        if spoke_inner_r < axle_r + 0.05:
            spoke_inner_r = axle_r + 0.05

        # ========== コンポーネントを作成 ==========
        occ = rootComp.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        wheelComp = occ.component
        wheelComp.name = "Wheel_Front"

        sketches = wheelComp.sketches
        xyPlane = wheelComp.xYConstructionPlane
        extrudes = wheelComp.features.extrudeFeatures
        centerPoint = adsk.core.Point3D.create(0, 0, 0)

        # ========== ステップ1: 外周リングを作成 ==========
        sketch1 = sketches.add(xyPlane)
        sketch1.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, outer_r)
        sketch1.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, inner_ring_r)

        ringProfile = None
        for prof in sketch1.profiles:
            area = prof.areaProperties().area
            outer_area = math.pi * outer_r * outer_r
            inner_area = math.pi * inner_ring_r * inner_ring_r
            ring_area = outer_area - inner_area
            if abs(area - ring_area) < 0.001:
                ringProfile = prof
                break

        if ringProfile:
            extInput = extrudes.createInput(ringProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
            extInput.setDistanceExtent(False, adsk.core.ValueInput.createByReal(thickness))
            extrudes.add(extInput)

        # ========== ステップ2: 中心ハブを作成 ==========
        sketch2 = sketches.add(xyPlane)
        sketch2.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, hub_r)

        hubProfile = sketch2.profiles.item(0)
        extInput2 = extrudes.createInput(hubProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
        extInput2.setDistanceExtent(False, adsk.core.ValueInput.createByReal(thickness))
        extrudes.add(extInput2)

        # ========== ステップ3: スポークを作成 ==========
        for i in range(num_spokes):
            angle = math.radians(i * (360 / num_spokes))

            sketch3 = sketches.add(xyPlane)
            lines = sketch3.sketchCurves.sketchLines

            cos_a = math.cos(angle)
            sin_a = math.sin(angle)
            cos_perp = math.cos(angle + math.pi/2)
            sin_perp = math.sin(angle + math.pi/2)

            half_w = spoke_w / 2

            p1 = adsk.core.Point3D.create(
                spoke_inner_r * cos_a - half_w * cos_perp,
                spoke_inner_r * sin_a - half_w * sin_perp, 0)
            p2 = adsk.core.Point3D.create(
                spoke_inner_r * cos_a + half_w * cos_perp,
                spoke_inner_r * sin_a + half_w * sin_perp, 0)
            p3 = adsk.core.Point3D.create(
                (inner_ring_r + overlap) * cos_a + half_w * cos_perp,
                (inner_ring_r + overlap) * sin_a + half_w * sin_perp, 0)
            p4 = adsk.core.Point3D.create(
                (inner_ring_r + overlap) * cos_a - half_w * cos_perp,
                (inner_ring_r + overlap) * sin_a - half_w * sin_perp, 0)

            lines.addByTwoPoints(p1, p2)
            lines.addByTwoPoints(p2, p3)
            lines.addByTwoPoints(p3, p4)
            lines.addByTwoPoints(p4, p1)

            spokeProfile = sketch3.profiles.item(0)
            extInput3 = extrudes.createInput(spokeProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
            extInput3.setDistanceExtent(False, adsk.core.ValueInput.createByReal(thickness))
            extrudes.add(extInput3)

        # ========== ステップ4: 全てのボディを結合 ==========
        bodies = wheelComp.bRepBodies
        bodyCount = bodies.count

        if bodyCount > 1:
            combineFeatures = wheelComp.features.combineFeatures
            targetBody = bodies.item(0)

            toolBodies = adsk.core.ObjectCollection.create()
            for i in range(1, bodyCount):
                toolBodies.add(bodies.item(i))

            combineInput = combineFeatures.createInput(targetBody, toolBodies)
            combineInput.operation = adsk.fusion.FeatureOperations.JoinFeatureOperation
            combineInput.isKeepToolBodies = False
            combineFeatures.add(combineInput)

        # ========== ステップ5: 軸穴を開ける ==========
        sketch4 = sketches.add(xyPlane)
        sketch4.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, axle_r)

        holeProfile = sketch4.profiles.item(0)
        extInput4 = extrudes.createInput(holeProfile, adsk.fusion.FeatureOperations.CutFeatureOperation)
        extInput4.setDistanceExtent(False, adsk.core.ValueInput.createByReal(thickness + 0.1))
        extrudes.add(extInput4)

        # ビューをフィット
        viewport = app.activeViewport
        viewport.fit()

        ui.messageBox(f'スポーク型ホイール（コンポーネント）完成！\n\n外径: {outer_diameter}mm\n軸穴: {axle_hole}mm\n厚み: {wheel_thickness}mm\nスポーク: {num_spokes}本\n\nコンポーネント名: Wheel_Front\n\nコピーして複数配置できます！')

    except:
        if ui:
            ui.messageBox('エラーが発生しました:\n{}'.format(traceback.format_exc()))
