# Fusion 360 スクリプト - 前輪用軸（Axle）
# 短い軸をホイールごとに使用

import adsk.core, adsk.fusion, adsk.cam, traceback, math

def run(context):
    ui = None
    try:
        app = adsk.core.Application.get()
        ui = app.userInterface
        design = app.activeProduct
        rootComp = design.rootComponent

        # === パラメータ設定（mm） ===
        axle_diameter = 5        # 軸の直径
        axle_length = 40         # 軸の長さ（ホイール25mm + 軸受け10mm + 余裕5mm）
        stopper_diameter = 8     # ストッパーの直径
        stopper_thickness = 2    # ストッパーの厚み

        # 単位をcmに変換
        axle_r = (axle_diameter / 2) / 10
        axle_len = axle_length / 10
        stopper_r = (stopper_diameter / 2) / 10
        stopper_t = stopper_thickness / 10

        # ========== コンポーネントを作成 ==========
        occ = rootComp.occurrences.addNewComponent(adsk.core.Matrix3D.create())
        axleComp = occ.component
        axleComp.name = "Axle_Front"

        sketches = axleComp.sketches
        xyPlane = axleComp.xYConstructionPlane
        extrudes = axleComp.features.extrudeFeatures
        centerPoint = adsk.core.Point3D.create(0, 0, 0)

        # ========== 軸本体を作成 ==========
        sketch1 = sketches.add(xyPlane)
        sketch1.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, axle_r)

        axleProfile = sketch1.profiles.item(0)
        extInput = extrudes.createInput(axleProfile, adsk.fusion.FeatureOperations.NewBodyFeatureOperation)
        extInput.setDistanceExtent(False, adsk.core.ValueInput.createByReal(axle_len))
        extrudes.add(extInput)

        # ========== ストッパー（端）を作成 ==========
        # 軸の端にストッパーを付ける（抜け防止）
        sketch2 = sketches.add(xyPlane)
        sketch2.sketchCurves.sketchCircles.addByCenterRadius(centerPoint, stopper_r)

        stopperProfile = sketch2.profiles.item(0)
        extInput2 = extrudes.createInput(stopperProfile, adsk.fusion.FeatureOperations.JoinFeatureOperation)
        extInput2.setDistanceExtent(False, adsk.core.ValueInput.createByReal(stopper_t))
        extrudes.add(extInput2)

        # ビューをフィット
        viewport = app.activeViewport
        viewport.fit()

        ui.messageBox(f'前輪用軸（Axle）完成！\n\n軸直径: {axle_diameter}mm\n軸長さ: {axle_length}mm\nストッパー直径: {stopper_diameter}mm\n\nコンポーネント名: Axle_Front')

    except:
        if ui:
            ui.messageBox('エラーが発生しました:\n{}'.format(traceback.format_exc()))
