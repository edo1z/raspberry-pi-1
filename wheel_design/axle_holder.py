# Fusion 360 スクリプト - 軸受け（Axle Holder）
# シャーシに取り付けて軸を支える筒

import adsk.core, adsk.fusion, adsk.cam, traceback, math

def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        # === パラメータ設定（mm） ===
        inner_diameter = 5.5     # 内径（軸5mm + 余裕0.5mm）
        outer_diameter = 12      # 外径
        holder_length = 15       # 軸受けの長さ
        mount_width = 20         # 取付部の幅
        mount_height = 5         # 取付部の高さ（シャーシに固定する部分）
        mount_hole_diameter = 3  # 取付穴の直径

        # 単位をcmに変換
        inner_r = (inner_diameter / 2) / 10
        outer_r = (outer_diameter / 2) / 10
        length = holder_length / 10
        m_width = mount_width / 10
        m_height = mount_height / 10
        m_hole_r = (mount_hole_diameter / 2) / 10

        # ========== コンポーネントを作成 ==========
        occ = rootComp.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        holderComp = occ.component
        holderComp.name = "Axle_Holder"

        sketches = holderComp.sketches
        xyPlane = holderComp.xYConstructionPlane
        xzPlane = holderComp.xZConstructionPlane
        extrudes = holderComp.features.extrudeFeatures
        centerPoint = adsk.core.Point3D.create(0, 0, 0)

        # ========== 筒部分を作成 ==========
        sketch1 = sketches.add(xyPlane)
        sketch1.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, outer_r)
        sketch1.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, inner_r)

        # リング状のプロファイルを見つける
        tubeProfile = None
        for prof in sketch1.profiles:
            area = prof.areaProperties().area
            outer_area = math.pi * outer_r * outer_r
            inner_area = math.pi * inner_r * inner_r
            ring_area = outer_area - inner_area
            if abs(area - ring_area) < 0.0001:
                tubeProfile = prof
                break

        if tubeProfile:
            extInput = extrudes.createInput(tubeProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
            extInput.setDistanceExtent(False, adsk.core.ValueInput.createByReal(length))
            extrudes.add(extInput)

        # ========== 取付部（フランジ）を作成 ==========
        sketch2 = sketches.add(xyPlane)
        lines = sketch2.sketchCurves.sketchLines

        # 取付部の四角形（筒の下に配置）
        half_w = m_width / 2
        p1 = adsk.core.Point3D.create(-half_w, -outer_r - m_height, 0)
        p2 = adsk.core.Point3D.create(half_w, -outer_r - m_height, 0)
        p3 = adsk.core.Point3D.create(half_w, -outer_r + 0.01, 0)
        p4 = adsk.core.Point3D.create(-half_w, -outer_r + 0.01, 0)

        lines.addByTwoPoints(p1, p2)
        lines.addByTwoPoints(p2, p3)
        lines.addByTwoPoints(p3, p4)
        lines.addByTwoPoints(p4, p1)

        mountProfile = sketch2.profiles.item(0)
        extInput2 = extrudes.createInput(mountProfile, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        extInput2.setDistanceExtent(False, adsk.core.ValueInput.createByReal(length))
        extrudes.add(extInput2)

        # ========== 取付穴を開ける ==========
        # 左側の穴
        sketch3 = sketches.add(xyPlane)
        holeCenter1 = adsk.core.Point3D.create(-half_w + m_hole_r + 0.02, -outer_r - m_height / 2, 0)
        sketch3.sketchCurves.sketchCircles.addByCenterRadius(holeCenter1, m_hole_r)

        holeProfile1 = sketch3.profiles.item(0)
        extInput3 = extrudes.createInput(holeProfile1, adsk.fusion.FeatureOperations.CutFeatureOperation)
        extInput3.setDistanceExtent(False, adsk.core.ValueInput.createByReal(length + 0.1))
        extrudes.add(extInput3)

        # 右側の穴
        sketch4 = sketches.add(xyPlane)
        holeCenter2 = adsk.core.Point3D.create(half_w - m_hole_r - 0.02, -outer_r - m_height / 2, 0)
        sketch4.sketchCurves.sketchCircles.addByCenterRadius(holeCenter2, m_hole_r)

        holeProfile2 = sketch4.profiles.item(0)
        extInput4 = extrudes.createInput(holeProfile2, adsk.fusion.FeatureOperations.CutFeatureOperation)
        extInput4.setDistanceExtent(False, adsk.core.ValueInput.createByReal(length + 0.1))
        extrudes.add(extInput4)

        # ビューをフィット
        viewport = app.activeViewport
        viewport.fit()

        ui.messageBox(f'軸受け（Axle Holder）完成！\n\n内径: {inner_diameter}mm（軸が通る）\n外径: {outer_diameter}mm\n長さ: {holder_length}mm\n取付部幅: {mount_width}mm\n\nコンポーネント名: Axle_Holder')

    except:
        if ui:
            ui.messageBox('エラーが発生しました:\n{}'.format(traceback.format_exc()))
